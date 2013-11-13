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
  ERROR("[Usage] trace2bset [Number of Traces] "
    "[.MemoryAccesses File Prefix] [.AccessesInfo File] [The Output .BSet File]");
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    print_help();
  }

  int cnt = atoi(argv[1]);
  map<uint64_t, AccessInfo> accessInfoMap;
  ReadAccessesInfoFile(argv[3], accessInfoMap);

  set<AI_ACCESS_ID> sharedAccesses;
  map<AI_ACCESS_ID, BSet> database;

  for (int I=0; I<cnt; ++I) { // One Trace
    AI_TID tid; AI_ACCESS_ID accessID; AI_ADDR *addr;
    char fName[AI_FILE_NAME_MAX_SIZE];
    sprintf(fName, "%s.%d", argv[2], I);
    FILE *trace = fopen(fName, "rb");
    map<AI_ADDR, RPreSet> recorder;
    
    map<AI_ADDR, set<AI_TID> > accessedThreads;
    map<AI_ADDR, set<AI_ACCESS_ID> > accessedIDs;

    while (fread(&tid, sizeof(tid), 1, trace) > 0) {
      assert(fread(&accessID, sizeof(accessID), 1, trace) > 0);
      assert(fread(&addr, sizeof(addr), 1, trace) > 0);

      // printf("MemoryAccess: %lu %lu %p\n", tid, accessID, addr);

      map<uint64_t, AccessInfo>::iterator info = accessInfoMap.find(accessID);
      if (accessInfoMap.end() == info) {
        ERROR("Unknow access ID");
      }

      for (uint32_t i=0; i<info->second.TypeSize; i+=AI_MEMORY_BASE) {
        set<AI_TID> *ptrTidSet = &(accessedThreads[addr+i]);
        ptrTidSet->insert(tid);
        set<AI_ACCESS_ID> *ptrIDSet = &(accessedIDs[addr+i]);
        ptrIDSet->insert(accessID);

        RPreSet *ptrRPreSet = &(recorder[addr+i]);
        AI_ACCESS_ID rPre = ptrRPreSet->FetchAndSet(tid, accessID);

        BSet *ptrBSet = &(database[accessID]);
        ptrBSet->Add(rPre);
      }
    }

    // Identify the shared memory accesses
    for (map<AI_ADDR, set<AI_TID> >::iterator it=accessedThreads.begin(); it!=accessedThreads.end(); ++it) {
      if (it->second.size() > 1) {
        set<AI_ACCESS_ID> *ptrIDSet = &(accessedIDs[it->first]);
        sharedAccesses.insert(ptrIDSet->begin(), ptrIDSet->end());
      }
    }
  }

  FILE *BSetFile = fopen(argv[4], "w");
  for (map<AI_ACCESS_ID, BSet>::iterator it=database.begin(); it!=database.end(); ++it) {
    if (sharedAccesses.count(it->first) == 0) 
      continue;

    fprintf(BSetFile, "%lu ", it->first);
    it->second.Print(BSetFile);
    fprintf(BSetFile, "\n");
  }
}