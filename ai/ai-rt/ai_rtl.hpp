#ifndef AI_TRL_HPP
#define AI_TRL_HPP

#include "ai-common/ai_structs.hpp"

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "pthread.h"
#include <atomic>
#include <sys/syscall.h>

namespace __ai {

// Interceptor

// TODO: __thread is not surpotted in Mac
static AI_TID __thread local_tid = 0;

// struct ThreadParam {
//   void* (*start_routine)(void *);
//   void *arg;

//   ThreadParam(void *(*a_) (void *), void *b_)
//     : start_routine(a_), arg(b_) {}
// };

// void * __ai_thread_start_func(void *v) {
//   ThreadParam *p = (ThreadParam*)v;
//   void* (*start_routine)(void *) = p->start_routine;
//   void *arg = p->arg;
//   free(p);

//   local_tid = pthread_self();
//   void *res = start_routine(arg);
//   // // Prevent the callback from being tail called,
//   // // it mixes up stack traces.
//   // volatile int foo = 42;
//   // foo++;
//   return res;
// }

// inline void InitializeInterceptor() {
// }

// inline int WrapperOfPthreadCreate(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
//   ThreadParam *p = (ThreadParam *) malloc(sizeof(ThreadParam));
//   (*p) = ThreadParam(start_routine, arg);
//   int res = pthread_create(thread, attr, __ai_thread_start_func, (void *) p);
//   return res;
// }

// Trace
FILE* TraceLog;
FILE* WhiteListLog;
atomic_flag TraceLogLock;

inline void TraceInitialize() {
  // Thread safe because done before all threads exist.
  static bool is_initialized = false;
  if (is_initialized) return;
  is_initialized = true;

  TraceLog = fopen("trace.bin", "wb");
  TraceLogLock.clear(std::memory_order_release);
  WhiteListLog = fopen("whitelist.mem", "wb");
  // InitializeInterceptor();
}

inline void TraceDestroy() {
  while(TraceLogLock.test_and_set(std::memory_order_acquire));
  if (TraceLog != NULL) {
    fclose(TraceLog);
    fclose(WhiteListLog);
    TraceLog = NULL;
    WhiteListLog = NULL;
  }
  TraceLogLock.clear(std::memory_order_release);
}

inline void TraceMemoryAccess(AI_ACCESS_ID id, AI_ADDR addr) {
  if (local_tid == 0) {
    local_tid = syscall(SYS_gettid);
  }

  while(TraceLogLock.test_and_set(std::memory_order_acquire));
  //printf("MemoryAccess: %lu %lu %p\n", local_tid, id, addr);

  fwrite(&local_tid, sizeof(local_tid), 1, TraceLog);
  fwrite(&id, sizeof(id), 1, TraceLog);
  fwrite(&addr, sizeof(void *), 1, TraceLog);
  // fflush(TraceLog);
  
  TraceLogLock.clear(std::memory_order_release);
}

inline void WrapperInsThisAddr(void *addr) {
  while(TraceLogLock.test_and_set(std::memory_order_acquire));
  fwrite(&addr, sizeof(void *), 1, WhiteListLog);
  TraceLogLock.clear(std::memory_order_release);
}

// Tolerate
size_t* BSetPtr = NULL;
AI_ACCESS_ID* BSetPool = NULL;
atomic_flag TolerateLogLock;
RPreRecorder *Recorder;
FILE* ViolationLog;

inline void initBSet() {
  FILE *f = fopen("sanitized-bset.bin", "rb");
  size_t sz;
  
  assert(fread(&sz, sizeof(size_t), 1, f) == 1);
  BSetPtr = (size_t*) malloc(sz * sizeof(size_t));
  assert(fread(BSetPtr, sizeof(size_t), sz, f) == sz);

  assert(fread(&sz, sizeof(size_t), 1, f) == 1);
  BSetPool = (size_t*) malloc(sz * sizeof(AI_ACCESS_ID));
  assert(fread(BSetPool, sizeof(AI_ACCESS_ID), sz, f) == sz);

  fclose(f);
}

inline void TolerateInitialize() {
  // Thread safe because done before all threads exist.
  static bool is_initialized = false;
  if (is_initialized) return;
  is_initialized = true;

  ViolationLog = fopen("violations.bin", "wb");
  TolerateLogLock.clear(std::memory_order_release);
  // InitializeInterceptor();
  initBSet();
  Recorder = (RPreRecorder *) malloc(sizeof(RPreRecorder));
  Recorder->Init(25); // Adjust this parameter to control the size of Recorder.
  fprintf(stderr, "AI Initialization Done!\n");
}

inline void TolerateDestroy() {
  while(TolerateLogLock.test_and_set(std::memory_order_acquire));
  if (ViolationLog != NULL) {
    fclose(ViolationLog);
    ViolationLog = NULL;

    if (BSetPtr != NULL) {
      free(BSetPtr);
      BSetPtr = NULL;
    }

    if (BSetPool != NULL) {
      free(BSetPool);
      BSetPool = NULL;
    }
    
    Recorder->Destroy();
  }
  TolerateLogLock.clear(std::memory_order_release);
}

#define TRY_THRESHOLD 20
#define TRY_USLEEP_TIME 100000

// Only check for finding violations, no stalling.
inline void RelaxMemoryAccess(AI_ACCESS_ID id, AI_ADDR addr) {
  if (local_tid == 0) {
    local_tid = syscall(SYS_gettid);
  }
  RPreRecorderItem *item = Recorder->GetItem(addr);
  AI_ACCESS_ID RPre = AI_ACCESS_ID_NULL;

  bool violation = true;
  while(item->lock.test_and_set(std::memory_order_acquire));

  RPre = item->set.GetRPre(local_tid, id);
  for (size_t i=BSetPtr[id-1]; i<BSetPtr[id]; ++i) {
    if (BSetPool[i] == RPre) {
      violation = false;
      break;
    }
  }
  if (BSetPtr[id-1] == BSetPtr[id]) violation = false;
  item->set.Set(local_tid, id);
  item->lock.clear(std::memory_order_release);
  
  if (violation) {
    while(TolerateLogLock.test_and_set(std::memory_order_acquire));
    fprintf(ViolationLog, "%d %lu %lu\n", 0, id, RPre);
    TolerateLogLock.clear(std::memory_order_release);
  }
}

inline void TolerateMemoryAccess(AI_ACCESS_ID id, AI_ADDR addr) {
  if (local_tid == 0) {
    local_tid = syscall(SYS_gettid);
  }
  int tryTimes = 0;
  RPreRecorderItem *item = Recorder->GetItem(addr);
  AI_ACCESS_ID RPre = AI_ACCESS_ID_NULL;

  for (;;) {
    bool violation = true;
    while(item->lock.test_and_set(std::memory_order_acquire));

    RPre = item->set.GetRPre(local_tid, id);
    for (size_t i=BSetPtr[id-1]; i<BSetPtr[id]; ++i) {
      if (BSetPool[i] == RPre) {
        violation = false;
        break;
      }
    }
    if (BSetPtr[id-1] == BSetPtr[id]) violation = false;
    if (!violation) item->set.Set(local_tid, id);

    item->lock.clear(std::memory_order_release);
    

    if (violation) {
      while(TolerateLogLock.test_and_set(std::memory_order_acquire));
      fprintf(stderr, "Violation: from Tread %lu Access %lu to Address %p (RPre: %lu, TryTime: %d)\n", 
        local_tid, id, addr, RPre, tryTimes);
      if (tryTimes == 0) {
        fprintf(ViolationLog, "%d %lu %lu\n", 0, id, RPre);
        fflush(ViolationLog);
      }
      TolerateLogLock.clear(std::memory_order_release);
    } else {
      break;
    }

    if (++tryTimes >= TRY_THRESHOLD) break;
    usleep(TRY_USLEEP_TIME);
  }

  if (tryTimes >= TRY_THRESHOLD) {
    while(TolerateLogLock.test_and_set(std::memory_order_acquire));
    fprintf(stderr, "Untolerable Violation: from Tread %lu Access %lu to Address %p (RPre: %lu, TryTime: %d)\n", 
        local_tid, id, addr, RPre, tryTimes);
    fprintf(ViolationLog, "%d %lu %lu\n", 1, id, RPre);
    fflush(ViolationLog);
    TolerateLogLock.clear(std::memory_order_release);
  }
}

} // namespace

#endif  // AI_TRL_HPP