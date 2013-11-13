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
  ERROR("[Usage:]\t sanitize-bset [.BSet File]\n"
    "[Output:]\t sanitized-bset.txt, sanitized-bset.bin");
}

map<AI_ACCESS_ID, AI_ACCESS_ID> M;
AI_ACCESS_ID CNT = 1;

AI_ACCESS_ID convert(AI_ACCESS_ID id) {
  if (id == AI_ACCESS_ID_NULL) return AI_ACCESS_ID_NULL;
  if (M.count(id) != 0)
    return M[id];
  M[id] = CNT++;
  return CNT - 1;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    print_help();
  }

  FILE *inf = fopen(argv[1], "r");
  
  AI_ACCESS_ID id; int cnt;
  set<AI_ACCESS_ID> S;
  while (fscanf(inf, "%lu", &id) != EOF) {
    convert(id); S.insert(id);
    assert(fscanf(inf, "%d", &cnt) != EOF);
    while (cnt--) {
      assert(fscanf(inf, "%lu", &id) != EOF);
      convert(id);
    }
  }
  fclose(inf);
  if (S.size() != M.size()) {
    ERROR("Incomplete BSet");
  }

  char fileName[AI_FILE_NAME_MAX_SIZE];

  // TXT File
  sprintf(fileName, "sanitized-bset.txt");
  FILE *outf = fopen(fileName, "w");
  inf = fopen(argv[1], "r");

  // Print the map
  fprintf(outf, "%zu\n", M.size());
  for (map<AI_ACCESS_ID, AI_ACCESS_ID>::iterator it=M.begin(); it!=M.end(); ++it) {
    fprintf(outf, "%lu %lu\n", it->first, it->second);
  }

  // Print Sanitized BSet
  while (fscanf(inf, "%lu", &id) != EOF) {
    fprintf(outf, "%lu", convert(id));

    assert(fscanf(inf, "%d", &cnt) != EOF);
    fprintf(outf, " %d", cnt);

    while (cnt--) {
      assert(fscanf(inf, "%lu", &id) != EOF);
      fprintf(outf, " %lu", convert(id));
    }

    fprintf(outf, "\n");
  }
  fclose(outf);
  fclose(inf);

  // Binary File
  vector<size_t> BSetPtr;
  vector<AI_ACCESS_ID> BSetPool;
  vector<vector<AI_ACCESS_ID> > tmp(M.size());
  sprintf(fileName, "sanitized-bset.bin");
  outf = fopen(fileName, "wb");
  inf = fopen(argv[1], "r");
  while (fscanf(inf, "%lu", &id) != EOF) {
    AI_ACCESS_ID _id = convert(id);
    assert(fscanf(inf, "%d", &cnt) != EOF);

    while (cnt--) {
      assert(fscanf(inf, "%lu", &id) != EOF);
      tmp[_id - 1].push_back(convert(id));
    }
  }
  BSetPtr.push_back(0);
  for (size_t i=0; i<tmp.size(); i++) {
    if (tmp[i].size() < 5) {
      for (size_t j=0; j<tmp[i].size(); j++)
        BSetPool.push_back(tmp[i][j]);
    } //TRICK
    BSetPtr.push_back(BSetPool.size());
  }

  size_t sz;
  sz = BSetPtr.size();
  fwrite(&sz, sizeof(size_t), 1, outf);
  fwrite(&(BSetPtr[0]), sizeof(BSetPtr[0]), BSetPtr.size(), outf);
  sz = BSetPool.size();
  fwrite(&sz, sizeof(size_t), 1, outf);
  fwrite(&(BSetPool[0]), sizeof(BSetPool[0]), BSetPool.size(), outf);

  fclose(outf);
  fclose(inf);
}