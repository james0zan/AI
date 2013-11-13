#ifndef TEST_COMMON_HPP
#define TEST_COMMON_HPP

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/DebugLoc.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Pass.h"
#include "llvm/DebugInfo.h"
#include "llvm/IR/Attributes.h"

#include "ai-common/ai_defines.h"

static Function *checkInterfaceFunction(Constant *FuncOrBitcast) {
  if (Function *F = dyn_cast<Function>(FuncOrBitcast))
     return F;
  FuncOrBitcast->dump();
  report_fatal_error("Instrumentation function redefined");
}

#endif  // TEST_COMMON_HPP