#include "ai-common/ai_defines.h"
#include "ai-common/ai_structs.hpp"
#include "ai-common/ai_helper.hpp"

#include <stdlib.h> 
#include <set>
#include <string>
#include <map>
#include <fstream>

using namespace std;
using namespace __ai;

void ERROR(const char *str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
}

void print_help() {
  ERROR("[Usage] read-violations [.AccessesInfo File] [Sanitized BSet Text File] [Violation Log File]");
}

map<uint64_t, AccessInfo> accessInfoMap;

inline void printAccess(AI_ACCESS_ID id) {
  printf("\n");
  if (id == AI_ACCESS_ID_NULL) {
    printf("(nil)\n");
    return;
  }

  printf("Access ID:\t%lu\n", id);
  printf("Type:\t%s\n", accessInfoMap[id].isWrite ? "Write":"Read");
  printf("Line:\t%u\n", accessInfoMap[id].Line);
  string F = accessInfoMap[id].Dir + "/" + accessInfoMap[id].File;
  printf("File:\t%s\n", F.c_str());
  printf("Content:\n");
  
  ifstream tmp(F);
  vector<string> tmpL;
  string s;
  while (getline(tmp, s)) {
    tmpL.push_back(s);
  }

  for (int i=max(1u, accessInfoMap[id].Line-5); i<=accessInfoMap[id].Line+5 && i<=tmpL.size(); i++)
    printf("%s%s\n", i==accessInfoMap[id].Line?"**":"  ", tmpL[i-1].c_str());
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    print_help();
  }
  
  ReadAccessesInfoFile(argv[1], accessInfoMap);

  map<AI_ACCESS_ID, AI_ACCESS_ID> IDVMap;
  map<AI_ACCESS_ID, vector<AI_ACCESS_ID> > BSet;
  FILE *AISanitizedBSetFile = fopen(argv[2], "r");

  size_t sz;
  assert(fscanf(AISanitizedBSetFile, "%zu", &sz) != EOF);
  AI_ACCESS_ID a,b;
  while (sz--) {
    assert(fscanf(AISanitizedBSetFile, "%lu%lu", &a, &b) != EOF);
    IDVMap[b] = a;
  }

  while (fscanf(AISanitizedBSetFile, "%lu", &a) != EOF) {
     assert(fscanf(AISanitizedBSetFile, "%zu", &sz) != EOF);
     BSet[a];
     while (sz--) {
      assert(fscanf(AISanitizedBSetFile, "%lu", &b) != EOF);
       BSet[a].push_back(b);
     }
  }

  FILE * violations = fopen(argv[3], "r");
  int type; 
  while (fscanf(violations, "%d%lu%lu", &type, &a, &b) != EOF) {
    printf("--------------------------\n %s Violation \n--------------------------\n\n",
      type == 1? "Untolerable" : "Tolerated");

    printf(">> The RPre of Access:\n");
    printAccess(IDVMap[a]);

    printf("\n>> Is:\n");
    printAccess(IDVMap[b]);

    printf("\n>> Not Belongs to Its BSet:\n");
    for (size_t i=0; i<BSet[a].size(); i++)
      printAccess(IDVMap[BSet[a][i]]);

    puts("");
  }
}