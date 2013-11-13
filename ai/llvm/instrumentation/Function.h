#ifndef INSTRUMENTATION_FUNCTION_H
#define INSTRUMENTATION_FUNCTION_H

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"

#include "vector"

using namespace std;
using namespace llvm;

namespace llvm_instrumentation {

struct InstrumentFunctionMonitor {
  // Manipulate
  void addToGlobalCtors(Module &M, StringRef FunctionName, Type *RetTy, ...);
  void addToGlobalDtors(Module &M, StringRef FunctionName, Type *RetTy, ...);

  // Instrument

  // Replace
  struct functionReplacementPair {
    StringRef OriginalFunctionName;
    Function *ReplacingFunction;

    functionReplacementPair(StringRef a_, Function *b_)
      :OriginalFunctionName(a_), ReplacingFunction(b_) {}
  };
  vector<functionReplacementPair> functionReplacements;

  void RegisterFunctionReplacement(Module &M, Function *OriginalFunction, StringRef ReplacingFunctionName);
  void RegisterFunctionReplacement(Module &M, StringRef OriginalFunction, StringRef ReplacingFunction, Type *RetTy, ...);
  bool replaceFunction(Function &F, Function *ReplacingFunction);

  bool Instrument(Function &F);
};

} // namespace

#endif // INSTRUMENTATION_FUNCTION_H