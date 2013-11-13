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
  ERROR("[Usage] read-ins-group [.AccessesInfo File] [.Group File]");
}

map<uint64_t, AccessInfo> accessInfoMap;

inline void printAccess(AI_ACCESS_ID id) {
  printf("\n");
  if (id == AI_ACCESS_ID_NULL) {
    printf("(nil)");
    return;
  }

  printf("Access ID:\t%lu\n", id);
  printf("Type:\t%s\n", accessInfoMap[id].isWrite ? "Write":"Read");
  printf("Line:\t%u\n", accessInfoMap[id].Line);
  string F = accessInfoMap[id].Dir + "/" + accessInfoMap[id].File;
  printf("File:\t%s\n", F.c_str());
  printf("Content:\n");
  
  ifstream tmp(F);
  if (! tmp.is_open()) {
    printf("ERROR\n");
    return;
  }
  vector<string> tmpL;
  string s;
  while (getline(tmp, s)) {
    tmpL.push_back(s);
  }

  for (int i=max(1u, accessInfoMap[id].Line-5); i<=min(accessInfoMap[id].Line+5, (unsigned)tmpL.size()-1); i++)
    printf("%s%s\n", i==accessInfoMap[id].Line?"**":"  ", tmpL[i-1].c_str());
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    print_help();
  }

  ReadAccessesInfoFile(argv[1], accessInfoMap);

  FILE *insGroupFile = fopen(argv[2], "r");
  int cnt = 1, tot;
  while (fscanf(insGroupFile, "%d", &tot) != EOF) {
    printf("==========================\n Group %d \n==========================\n", cnt++);
    AI_ACCESS_ID ins;
    while (tot--) {
      assert(fscanf(insGroupFile, "%lu", &ins) != EOF);
      printAccess(ins);
    }
    double p, p2;
    assert(fscanf(insGroupFile, "%lf%lf", &p, &p2) != EOF);
    printf("\nProportion: %.20lf\t%.20lf\n\n", p, p2);
  }
}