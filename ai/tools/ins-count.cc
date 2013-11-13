#include "ai-common/ai_defines.h"
#include "ai-common/ai_structs.hpp"
#include "ai-common/ai_helper.hpp"

#include <stdlib.h> 
#include <set>

using namespace std;
using namespace __ai;

void ERROR(const char *str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
}

void print_help() {
  ERROR("[Usage] ins-count [Number of Traces] "
    "[.MemoryAccesses File Prefix] [.BSet File]");
}

struct SS {
  pair<int, uint64_t> pa;
  int rank;
  SS() {}
  SS(pair<int, uint64_t> _pa, int _rank) :pa(_pa), rank(_rank) {}
};

map<pair<int, uint64_t>, SS > DisjointSet;

SS find_parent(pair<int, uint64_t> a, int depth) {
  SS s(a, 0);
  if (DisjointSet.count(a) == 0) {
    DisjointSet[a] = s;
    return s;
  }

  s = DisjointSet[a];
  if (s.pa == a)
    return s;

  s.pa = find_parent(s.pa, depth+1).pa;
  DisjointSet[a] = s;
  return s;
}

void add_edge(pair<int, uint64_t> a, pair<int, uint64_t> b) {
  SS paA = find_parent(a, 0);
  SS paB = find_parent(b, 0);
  if (paB.pa == paA.pa) return;
  // fprintf(stderr, "Father of %d %lu is %d %lu\n", a.first, a.second, paA.pa.first, paA.pa.second);
  // fprintf(stderr, "Father of %d %lu is %d %lu\n", b.first, b.second, paB.pa.first, paB.pa.second);

  if (paA.rank < paB.rank) {
    // paA.pa = paB.pa;
    DisjointSet[paA.pa] = SS(paB.pa, paA.rank);
  } else if (paA.rank > paB.rank) {
    DisjointSet[paB.pa] = SS(paA.pa, paB.rank);
  } else {
    DisjointSet[paB.pa] = SS(paA.pa, paB.rank);
    DisjointSet[paA.pa] = SS(paA.pa, paA.rank + 1);
  }
  // fprintf(stderr, "Cahnge Father of %d %lu to %d %lu\n", paA.pa.first, paA.pa.second, DisjointSet[paA.pa].pa.first, DisjointSet[paA.pa].pa.second);
  // fprintf(stderr, "Cahnge Father of %d %lu to %d %lu\n", paB.pa.first, paB.pa.second, DisjointSet[paB.pa].pa.first, DisjointSet[paB.pa].pa.second);
  // fprintf(stderr, "\n");
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    print_help();
  }
  FILE *inf = fopen(argv[3], "r");
  
  AI_ACCESS_ID id; int cnt;
  set<AI_ACCESS_ID> S;
  while (fscanf(inf, "%lu", &id) != EOF) {
    S.insert(id);
    assert(fscanf(inf, "%d", &cnt) != EOF);
    while (cnt--) {
      assert(fscanf(inf, "%lu", &id) != EOF);
    }
  }
  fclose(inf);

  map<AI_ACCESS_ID, double> Ratio, Ratio2;
  int N = atoi(argv[1]);
  DisjointSet.clear();

  set<AI_ACCESS_ID> whiteList;
  for (int I=0; I<N; ++I) { // One Trace
    AI_TID tid; AI_ACCESS_ID accessID; AI_ADDR *addr;
    char fName[AI_FILE_NAME_MAX_SIZE];
    sprintf(fName, "%s.%d", argv[2], I);
    FILE *trace = fopen(fName, "rb");
    sprintf(fName, "whitelist.mem.%d", I);
    FILE *memFile = fopen(fName, "rb");
    set<void *> mem;
    if (memFile != NULL) {
      void *addr;
      while (fread(&addr, sizeof(addr), 1, memFile) == 1) {
        mem.insert(addr);
      }
    }

    map<AI_ACCESS_ID, uint64_t> M;
    uint64_t tot = 0, tot2 = 0;

    while (fread(&tid, sizeof(tid), 1, trace) > 0) {
      assert(fread(&accessID, sizeof(accessID), 1, trace) > 0);
      assert(fread(&addr, sizeof(addr), 1, trace) > 0);
      if (S.count(accessID)) {
        M[accessID] = M[accessID] + 1;
        add_edge(make_pair(0, (uint64_t)accessID), make_pair(1 + I, (uint64_t)addr));
        tot++;

        if (mem.count(addr)) {
          whiteList.insert(accessID);
        }
      }
      tot2++;
    }

    for (map<AI_ACCESS_ID, uint64_t>::iterator it=M.begin(); it!=M.end(); ++it) {
      Ratio[it->first] += (((double)(it->second))/tot)/N;
      Ratio2[it->first] += (((double)(it->second))/tot2)/N;
    }
  }

  map<pair<int, uint64_t>, int> Sets;
  vector<vector<AI_ACCESS_ID> > Groups;
  for (map<pair<int, uint64_t>, SS >::iterator it=DisjointSet.begin(); it!=DisjointSet.end(); ++it) {
    if (it->first.first == 0) {
      pair<int, uint64_t> fa = find_parent(it->first, 0).pa;
      if (Sets.count(fa) == 0) {
        int sz = Sets.size();
        Sets[fa] = sz;
      }
    }
  }

  Groups.resize(Sets.size());
  for (map<pair<int, uint64_t>, SS >::iterator it=DisjointSet.begin(); it!=DisjointSet.end(); ++it) {
    if (it->first.first == 0) {
      pair<int, uint64_t> fa = find_parent(it->first, 0).pa;
      Groups[Sets[fa]].push_back((AI_ACCESS_ID)(it->first.second));
    }
  }

  for (size_t i=0; i<Groups.size(); i++) {
    printf("%zu", Groups[i].size());
    double p = 0, p2 = 0;
    bool inWhiteList = false;
    for (size_t j=0; j<Groups[i].size(); j++) {
      printf(" %lu", Groups[i][j]);
      p += Ratio[Groups[i][j]];
      p2 += Ratio2[Groups[i][j]];
      if (whiteList.count(Groups[i][j]))
        inWhiteList = true;
    }
    if (inWhiteList) {
      p = -1;
    }
    printf(" %.20lf %.20lf\n", p, p2);
  }
}