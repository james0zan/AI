#include "instrumentation/Function.h"

#include "common.hpp"
#include <set>

using namespace std;
using namespace llvm;
using namespace llvm_instrumentation;

static cl::opt<std::string> AIBSet("AIBSet", cl::init(""),
       cl::desc("BSet File"), cl::Hidden);

namespace {
  struct AITrace : public ModulePass {
    static char ID;
    AITrace() : ModulePass(ID) {}

    InstrumentFunctionMonitor iFMonitor;
    Function *AITraceMemoryAccess;

    bool runOnModule(Module &M);

    bool instrumentLoadOrStore(Instruction *I);
    set<AI_ACCESS_ID> shareAccesses;
  }; // struct AITrace
} // namespace

char AITrace::ID = 0;
static RegisterPass<AITrace> X("AITrace", "AI Trace Pass");

bool AITrace::runOnModule(Module &M) {
  IRBuilder<> IRB(M.getContext());

  shareAccesses.clear();
  if (AIBSet != "") {
    FILE *inf = fopen(AIBSet.c_str(), "r");
    AI_ACCESS_ID id; int cnt;
    while (fscanf(inf, "%lu", &id) != EOF) {
      shareAccesses.insert(id);
      assert(fscanf(inf, "%d", &cnt) != EOF);
      while (cnt--) {
        assert(fscanf(inf, "%lu", &id) != EOF);
        shareAccesses.insert(id);
      }
    }
  }

  iFMonitor.addToGlobalCtors(M, "__ai_trace_init", IRB.getVoidTy(), NULL);
  iFMonitor.addToGlobalDtors(M, "__ai_trace_destroy", IRB.getVoidTy(), NULL);

  Function *aiInsThisAddr = M.getFunction("AI_INS_THIS_ADDR");
  if (aiInsThisAddr) {
    iFMonitor.RegisterFunctionReplacement(M, aiInsThisAddr, "__ai_wrapper_ins_this_addr");
  }

  Module::FunctionListType &functions = M.getFunctionList();
  for (Module::FunctionListType::iterator it = functions.begin(), it_end = functions.end(); it != it_end; ++it) {
    iFMonitor.Instrument(*it);
  }

  AITraceMemoryAccess = checkInterfaceFunction(M.getOrInsertFunction("__ai_trace_memory_access", IRB.getVoidTy(), 
    IRB.getInt64Ty(), IRB.getInt8PtrTy(), NULL));
  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    Function &F =*MI;
    for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
      BasicBlock &BB = *FI;
      for (BasicBlock::iterator BI = BB.begin(), BE = BB.end(); BI != BE; ++BI) {
        if (isa<LoadInst>(BI) || isa<StoreInst>(BI))
          instrumentLoadOrStore(BI);
      }
    }
  }

  return true;
}

bool AITrace::instrumentLoadOrStore(Instruction *I) {
  IRBuilder<> IRB(I);
  
  MDNode *Node = I->getMetadata("AIMemoryAccessID");
  if (!Node) {
    // HACK: Do not instrument instruction that has no AIMemoryAccessID
    return false;
  }

  Value *ID = Node->getOperand(0);
  ConstantInt *CI = dyn_cast<ConstantInt>(ID);
  if (!CI) {
    report_fatal_error("Error AIMemoryAccessID!");
  }
  if (shareAccesses.size() > 0) {
    if (shareAccesses.count(CI->getZExtValue()) == 0)
      return false;
  }
  
  // Add a Call Before it
  bool IsWrite = isa<StoreInst>(*I);
  Value *Addr = IsWrite
      ? cast<StoreInst>(I)->getPointerOperand()
      : cast<LoadInst>(I)->getPointerOperand();
  
  IRB.CreateCall2(AITraceMemoryAccess, 
    CI, 
    IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()));

  return true;
}
