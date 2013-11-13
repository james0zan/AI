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
  ERROR("[Usage] custom-trace2bset [Number of Traces] "
    "[.MemoryAccesses File Prefix] [.AccessesInfo File] "
    "[AI Ins Group File] [AI Ins Group Threshold] "
    "[Whitelist Ins File] [The Output .BSet File]");
}

int main(int argc, char *argv[]) {
  if (argc != 8) {
    print_help();
  }

  int cnt = atoi(argv[1]);
  map<uint64_t, AccessInfo> accessInfoMap;
  ReadAccessesInfoFile(argv[3], accessInfoMap);

  set<AI_ACCESS_ID> whiteList;
  FILE *whiteListFile = fopen(argv[6], "r");
  if (whiteListFile) {
    AI_ACCESS_ID ID;
    while (fscanf(whiteListFile, "%lu", &ID) != EOF)
      whiteList.insert(ID);
  }

  FILE *groupFile = fopen(argv[4], "r");
  double groupThreshold = atof(argv[5]);
  int tot;
  while (fscanf(groupFile, "%d", &tot) != EOF) {
    AI_ACCESS_ID ins;
    vector<AI_ACCESS_ID> v;
    while (tot--) {
      assert(fscanf(groupFile, "%lu", &ins) != EOF);
      v.push_back(ins);
    }
    double p, p2;

    assert(fscanf(groupFile, "%lf%lf", &p, &p2) != EOF);
    if (p < groupThreshold) {
      whiteList.insert(v.begin(), v.end());
    }
  }

  set<AI_ACCESS_ID> sharedAccesses;
  map<AI_ACCESS_ID, BSet> database;

  for (int I=0; I<cnt; ++I) { // One Trace
    AI_TID tid; AI_ACCESS_ID accessID; AI_ADDR *addr;
    char fName[AI_FILE_NAME_MAX_SIZE];
    sprintf(fName, "%s.%d", argv[2], I);
    FILE *trace = fopen(fName, "rb");
    map<AI_ADDR, RPreSet> recorder;

    while (fread(&tid, sizeof(tid), 1, trace) > 0) {
      assert(fread(&accessID, sizeof(accessID), 1, trace) > 0);
      assert(fread(&addr, sizeof(addr), 1, trace) > 0);

      if (whiteList.count(accessID) == 0) continue;
      
      map<uint64_t, AccessInfo>::iterator info = accessInfoMap.find(accessID);
      if (accessInfoMap.end() == info) {
        ERROR("Unknow access ID");
      }

      for (uint32_t i=0; i<info->second.TypeSize; i+=AI_MEMORY_BASE) {
        RPreSet *ptrRPreSet = &(recorder[addr+i]);
        AI_ACCESS_ID rPre = ptrRPreSet->FetchAndSet(tid, accessID);

        BSet *ptrBSet = &(database[accessID]);
        ptrBSet->Add(rPre);
      }
    }
  }

  FILE *BSetFile = fopen(argv[7], "w");
  for (map<AI_ACCESS_ID, BSet>::iterator it=database.begin(); it!=database.end(); ++it) {
    fprintf(BSetFile, "%lu ", it->first);
    it->second.Print(BSetFile);
    fprintf(BSetFile, "\n");
  }
}