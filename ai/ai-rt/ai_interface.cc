#include "ai-common/ai_defines.h"

#include "ai_interface.h"

#include "ai_rtl.hpp"

using namespace __ai;

void __ai_trace_init() {
  TraceInitialize();
}

void __ai_trace_destroy() {
  TraceDestroy();
}

void __ai_trace_memory_access(AI_ACCESS_ID id, AI_ADDR addr) {
  TraceMemoryAccess(id, addr);
}

void __ai_tolerate_init() {
  TolerateInitialize();
}

void __ai_tolerate_destroy() {
  TolerateDestroy();
}

void __ai_relax_memory_access(AI_ACCESS_ID id, AI_ADDR addr) {
  RelaxMemoryAccess(id, addr);
}

void __ai_tolerate_memory_access(AI_ACCESS_ID id, AI_ADDR addr) {
  TolerateMemoryAccess(id, addr);
}

void __ai_wrapper_ins_this_addr(void *addr){
  WrapperInsThisAddr(addr);
}
// int __ai_wrapper_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
//     WrapperOfPthreadCreate(thread, attr, start_routine, arg);
// }