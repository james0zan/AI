include_directories(..)

add_llvm_loadable_module(LLVMAITrace
  AITrace.cc
  ${LLVMInstrumentation_SOURCE_DIR}/Function.cc
)

add_llvm_loadable_module(LLVMAITolerate
  AITolerate.cc
  ${LLVMInstrumentation_SOURCE_DIR}/Function.cc
)

add_llvm_loadable_module(LLVMAIPrepare
  AIPrepare.cc
)