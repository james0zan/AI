#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H

#include "stdint.h"
#include "pthread.h"

#ifdef __cplusplus
extern "C" {
#endif

// This function should be called at the very beginning of the process,
// i.e. before any instrumented code is executed.
void __ai_trace_init();
void __ai_tolerate_init();

void __ai_trace_destroy();
void __ai_tolerate_destroy();

// Instrumentation
void __ai_trace_memory_access(uint64_t id, void *addr);
void __ai_relax_memory_access(uint64_t id, void *addr);
void __ai_tolerate_memory_access(uint64_t id, void *addr);

// Interceptors
void __ai_wrapper_ins_this_addr(void *addr);
// int __ai_wrapper_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AI_INTERFACE_H