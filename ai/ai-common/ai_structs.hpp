#ifndef AI_STRUCTS_HPP
#define AI_STRUCTS_HPP

#include "ai_defines.h"

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "assert.h"

#include <string>
#include <vector>
#include <atomic>
#include <set>

using namespace std;

namespace __ai {

  struct AccessInfo {
    bool isWrite;
    uint32_t TypeSize;
    unsigned Line;
    string File;
    string Dir;

    AccessInfo() {}
    AccessInfo(int _isWrite, uint32_t _TypeSize, unsigned _Line, char _File[], char _Dir[])
      :isWrite(_isWrite != 0), TypeSize(_TypeSize), Line(_Line), File(_File), Dir(_Dir) {}
  };
  
  // RPre := the last access to this address that is from another thread.
  // Thus, we only need to record the last two instructions that access this memory address 
  // and are from different threads.
  // (RPre is the last access if it is from a different thread.
  // Otherwise, RPre must be the second last one). 
  struct RPreSet {
    AI_TID _tids[2];
    AI_ACCESS_ID _ids[2];

    RPreSet() {_tids[0] = AI_TID_NULL;}
    
    // If the Access id from Thread id accesses this address,
    // get the corresponding RPre and update the states.
    inline AI_ACCESS_ID FetchAndSet(AI_TID tid, AI_ACCESS_ID id) {
      AI_ACCESS_ID ans = GetRPre(tid, id);
      Set(tid, id);
      return ans;
    }

    inline AI_ACCESS_ID GetRPre(AI_TID tid, AI_ACCESS_ID id) {
      AI_ACCESS_ID ans = AI_ACCESS_ID_NULL;
      
      if (!(
            _tids[0] == AI_TID_NULL // The first access
            || (tid == _tids[0] && _tids[1] == AI_TID_NULL) // Only Thread tid has accessed this address
      )) {
        ans = (_tids[0] != tid) ? _ids[0] : _ids[1];
      }

      return ans;
    }

    inline void Set(AI_TID tid, AI_ACCESS_ID id) {
      if (tid != _tids[0]) {
        _tids[1] = _tids[0]; _tids[0] = tid; 
        _ids[1] = _ids[0]; _ids[0] = id;
      } else {
        _ids[0] = id;
      }
    }
  };

  struct RPreRecorderItem {
    AI_ADDR addr;
    RPreSet set;
    atomic_flag lock;
  };

  struct RPreRecorder {
    unsigned N, Mask;
    RPreRecorderItem *pool;

    void Init(int N_) {
      N = 1U << N_; Mask = N - 1;
      //printf("N: %u\n", N);
      pool = (RPreRecorderItem*) malloc(N * sizeof(RPreRecorderItem));
      assert(pool != NULL);
      for (int i=0; i<N; i++) {
        pool[i].addr = 0;
        pool[i].lock.clear(std::memory_order_release);
      }
    }

    void Destroy() {
      if (pool) {
        free(pool);
        pool = NULL;
      }
    }

    inline unsigned hash(long key) {
      key = (~key) + (key << 21); // key = (key << 21) - key - 1;
      key = key ^ (key >> 24);
      key = (key + (key << 3)) + (key << 8); // key * 265
      key = key ^ (key >> 14);
      key = (key + (key << 2)) + (key << 4); // key * 21
      key = key ^ (key >> 28);
      key = key + (key << 31);
      return (unsigned) (key & Mask);
    }

    inline RPreRecorderItem *GetItem(AI_ADDR addr) {
      unsigned i = hash((long) addr);
      // unsigned cnt = 1;
      for (bool done = false;;) {
        while(pool[i].lock.test_and_set(std::memory_order_acquire));
        if (pool[i].addr == 0 || pool[i].addr == addr) done = true;
        pool[i].lock.clear(std::memory_order_release);
        if (done) break;

        ++i; i &= Mask;
        // ++cnt;
      }
      // if (cnt > 3) {
      //     printf("Large cnt: %u\n", cnt);
      // }
      return &(pool[i]);
    }
  };

  struct BSet {
    std::vector<AI_ACCESS_ID> _set;
    BSet() {
      _set.reserve(AI_BSET_RESERVED_SIZE);
    }

    bool Contain(AI_ACCESS_ID id) {
      for (size_t i=0; i<_set.size(); i++)
        if (id == _set[i]) {
          // swap(_set[0], _set[i]);
          return true;
        }
      return false;
    }
    
    void Add(AI_ACCESS_ID id) {
      if (!Contain(id)) 
        _set.push_back(id);
    }

    void Print(FILE * f) {
      fprintf(f, "%zu", _set.size());
      std::set<AI_ACCESS_ID> ss;
      ss.insert(_set.begin(), _set.end());
      for (std::set<AI_ACCESS_ID>::iterator it=ss.begin(); it!=ss.end(); ++it)
        fprintf(f, " %lu", *it);
    }
  };
}

#endif // AI_STRUCTS_HPP