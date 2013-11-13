#ifndef AI_API_HPP
#define AI_API_HPP

// The functions defined in this file are only stubs,
// the true magic is done in the LLVM passes.

#define AI_INS_THIS_FUNC __ai_ins_this_func();
#define AI_INS_THIS_BB __ai_ins_this_bb();

#ifdef __cplusplus
extern "C" {
#endif
  void AI_INS_THIS_ADDR(void *addr) {
  }
  void __ai_ins_this_func() {
  }
  void __ai_ins_this_bb() {
  }
#ifdef __cplusplus
}  // extern "C"
#endif

#endif // AI_API_HPP