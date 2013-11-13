#include "Function.h"

#include "llvm/IR/Attributes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "stdarg.h"
#include "stdio.h"

using namespace std;
using namespace llvm;

using namespace llvm_instrumentation;

#define constructFunctionFromArgs(fun, M, FunctionName, RetTy) do {     \
  va_list Args;                                                         \
  va_start(Args, RetTy);                                                \
  vector<Type*> ArgTys;                                                 \
  while (Type *ArgTy = va_arg(Args, Type*))                             \
    ArgTys.push_back(ArgTy);                                            \
  va_end(Args);                                                         \
  fun = checkInterfaceFunction(                                         \
        M.getOrInsertFunction(                                          \
            FunctionName,                                               \
            FunctionType::get(RetTy, ArgTys, false), AttributeSet()));  \
} while(0)
  

static Function *checkInterfaceFunction(Constant *FuncOrBitcast) {
  if (Function *F = dyn_cast<Function>(FuncOrBitcast))
     return F;
  FuncOrBitcast->dump();
  report_fatal_error("Instrumentation function redefined");
}

void InstrumentFunctionMonitor::addToGlobalCtors(Module &M, StringRef FunctionName, Type *RetTy, ...) {
  Function *fun = NULL;
  constructFunctionFromArgs(fun, M, FunctionName, RetTy);
  appendToGlobalCtors(M, fun, 0);
}

void InstrumentFunctionMonitor::addToGlobalDtors(Module &M, StringRef FunctionName, Type *RetTy, ...) {
  Function *fun = NULL;
  constructFunctionFromArgs(fun, M, FunctionName, RetTy);
  appendToGlobalDtors(M, fun, 0);
}

void InstrumentFunctionMonitor::RegisterFunctionReplacement(
  Module &M,
  Function *OriginalFunction,
  StringRef ReplacingFunctionName) {

  Function * ReplacingFunction
    = checkInterfaceFunction(
        M.getOrInsertFunction(
            ReplacingFunctionName,
            OriginalFunction->getFunctionType(), AttributeSet()));
  
  functionReplacements.push_back(functionReplacementPair(OriginalFunction->getName(), ReplacingFunction));
}

void InstrumentFunctionMonitor::RegisterFunctionReplacement(
  Module &M,
  StringRef OriginalFunctionName,
  StringRef ReplacingFunctionName,
  Type *RetTy, ... ) {

  Function * ReplacingFunction = NULL;
  constructFunctionFromArgs(ReplacingFunction, M, ReplacingFunctionName, RetTy);
  functionReplacements.push_back(functionReplacementPair(OriginalFunctionName, ReplacingFunction));
}

bool InstrumentFunctionMonitor::replaceFunction(Function &F, Function *ReplacingFunction) {
  if (F.getFunctionType() != ReplacingFunction->getFunctionType()) {
    report_fatal_error("Instrumentation: the type of replcing function mismatches!");
  }

  bool is_change = false;

  while (!F.use_empty()) {
    CallSite CS(F.use_back());
    vector<Value *> args(CS.arg_begin(), CS.arg_end());
    Instruction *call = CS.getInstruction();
    Instruction *new_call = NULL;
    const AttributeSet &call_attr = CS.getAttributes();

    if (InvokeInst *ii = dyn_cast<InvokeInst>(call)) {
      new_call = InvokeInst::Create(ReplacingFunction, ii->getNormalDest(), ii->getUnwindDest(),
                                    args, "", call);
      InvokeInst *inv = cast<InvokeInst>(new_call);
      inv->setCallingConv(CS.getCallingConv());
      inv->setAttributes(call_attr);
    } else {
      new_call = CallInst::Create(ReplacingFunction, args, "", call);
      CallInst *ci = cast< CallInst >(new_call);
      ci->setCallingConv(CS.getCallingConv());
      ci->setAttributes(call_attr);
      if (ci->isTailCall())
        ci->setTailCall();
    }

    new_call->setDebugLoc(call->getDebugLoc());
    if (!call->use_empty()) {
      call->replaceAllUsesWith(new_call);
    }

    new_call->takeName(call);
    call->eraseFromParent();
    is_change = true;
  }

  return is_change;
}

bool InstrumentFunctionMonitor::Instrument(Function &F) {
  bool is_change = false;

  // Replacing
  for (vector<functionReplacementPair>::iterator it = functionReplacements.begin(); 
    it != functionReplacements.end(); ++it) {
    if (F.getName() == it->OriginalFunctionName) {
      is_change |= replaceFunction(F, it->ReplacingFunction);
    }
  }

  return is_change;
}