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
  ERROR("[Usage:]\t relax-bset [Sanitized BSet Text File] [Violations File]\n");
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    print_help();
  }

  map<AI_ACCESS_ID, AI_ACCESS_ID> IDVMap;
  map<AI_ACCESS_ID, AI_ACCESS_ID> IDMap;
  map<AI_ACCESS_ID, set<AI_ACCESS_ID> > BSet;
  FILE *AISanitizedBSetFile = fopen(argv[1], "r");

  IDMap[AI_ACCESS_ID_NULL] = IDVMap[AI_ACCESS_ID_NULL] = AI_ACCESS_ID_NULL;
  size_t sz;
  assert(fscanf(AISanitizedBSetFile, "%zu", &sz) != EOF);
  AI_ACCESS_ID a,b;
  while (sz--) {
    assert(fscanf(AISanitizedBSetFile, "%lu%lu", &a, &b) != EOF);
    IDVMap[b] = a;
    IDMap[a] = b;
  }

  while (fscanf(AISanitizedBSetFile, "%lu", &a) != EOF) {
     assert(fscanf(AISanitizedBSetFile, "%zu", &sz) != EOF);
     a = IDVMap[a];
     BSet[a];
     while (sz--) {
      assert(fscanf(AISanitizedBSetFile, "%lu", &b) != EOF);
       BSet[a].insert(IDVMap[b]);
     }
  }
  fclose(AISanitizedBSetFile);

  FILE * violations = fopen(argv[2], "r");
  int type; 
  while (fscanf(violations, "%d%lu%lu", &type, &a, &b) != EOF) {
    BSet[IDVMap[a]].insert(IDVMap[b]);
  }


  char fileName[AI_FILE_NAME_MAX_SIZE];

  // TXT File
  FILE *outf = fopen("sanitized-bset.txt", "w");
  fprintf(outf, "%zu\n", IDMap.size() - 1);
  for (map<AI_ACCESS_ID, AI_ACCESS_ID>::iterator it=IDMap.begin(); it!=IDMap.end(); ++it) {
    if (it->first == 0) continue;
    fprintf(outf, "%lu %lu\n", it->first, it->second);
  }
  for (map<AI_ACCESS_ID, set<AI_ACCESS_ID> >::iterator it=BSet.begin(); it!=BSet.end(); ++it) {
    fprintf(outf, "%lu ", IDMap[it->first]);
    fprintf(outf, "%zu", it->second.size());
    for (set<AI_ACCESS_ID>::iterator it2=it->second.begin(); it2!=it->second.end(); ++it2) {
      fprintf(outf, " %lu", IDMap[*it2]);
    }
    fprintf(outf, "\n");
  }
  fclose(outf);

  // Binary File
  vector<size_t> BSetPtr;
  vector<AI_ACCESS_ID> BSetPool;
  vector<vector<AI_ACCESS_ID> > tmp(IDMap.size());
  outf = fopen("sanitized-bset.bin", "wb");
  for (map<AI_ACCESS_ID, set<AI_ACCESS_ID> >::iterator it=BSet.begin(); it!=BSet.end(); ++it) {
    AI_ACCESS_ID _id = IDMap[it->first];
    for (set<AI_ACCESS_ID>::iterator it2=it->second.begin(); it2!=it->second.end(); ++it2) {
      tmp[_id - 1].push_back(IDMap[*it2]);
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

  sz = BSetPtr.size();
  fwrite(&sz, sizeof(size_t), 1, outf);
  fwrite(&(BSetPtr[0]), sizeof(BSetPtr[0]), BSetPtr.size(), outf);
  sz = BSetPool.size();
  fwrite(&sz, sizeof(size_t), 1, outf);
  fwrite(&(BSetPool[0]), sizeof(BSetPool[0]), BSetPool.size(), outf);

  fclose(outf);  
}