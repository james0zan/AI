#ifndef AI_HELPER_HPP
#define AI_HELPER_HPP

#include "ai-common/ai_defines.h"
#include "ai-common/ai_structs.hpp"

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "assert.h"
#include <string>
#include <map>

using namespace std;

namespace __ai {

  void ReadAccessesInfoFile(char *fname, map<AI_ACCESS_ID, AccessInfo> &m) {
    m.clear();
    FILE *f = fopen(fname, "r");
    if (f == NULL) {
      fprintf(stderr, "Cannot open the AccessesInfoFile!");
      exit(1);
    }

    AI_ACCESS_ID aiMemoryAccessID;
    int isWrite;
    uint32_t TypeSize;
    unsigned Line;
    char file[AI_FILE_NAME_MAX_SIZE] = "", dir[AI_FILE_NAME_MAX_SIZE] = "";

    while (fscanf(f, "%lu\t|\t%d\t|\t%u\t|\t%u\t|\t%s\t|\t%s\n",
      &aiMemoryAccessID, &isWrite, &TypeSize, &Line, file, dir) != EOF) {
      // printf("Read: %lu\t|\t%d\t|\t%u\t|\t%u\t|\t%s\t|\t%s\n",
      //   aiMemoryAccessID, isWrite, TypeSize, Line, file, dir);

      m[aiMemoryAccessID] = AccessInfo(isWrite, TypeSize, Line, file, dir);
      file[0] = dir[0] = 0;
    }
    // fprintf(stderr, "HERE\n");
  }

}
#endif  // AI_HELPER_HPP