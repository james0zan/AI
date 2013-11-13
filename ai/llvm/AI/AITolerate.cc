#include "instrumentation/Function.h"

#include "common.hpp"

#include "stdio.h"
#include <map>
#include <set>

using namespace std;
using namespace llvm;
using namespace llvm_instrumentation;

static cl::opt<std::string> AISanitizedBSetFile("AISanitizedBSetFile", cl::init("sanitized-bset.txt"),
       cl::desc("Sanitized BSet File"), cl::Hidden);

static cl::opt<std::string> AIInsGroupFile("AIInsGroupFile", cl::init(""),
       cl::desc("Instruction Group File"), cl::Hidden);

static cl::opt<double> AIInsGroupThreshold("AIInsGroupThreshold", cl::init(0.4),
       cl::desc("Threshold of Instruction Group"), cl::Hidden);

static cl::opt<int> AIRelax("AIRelax", cl::init(0),
       cl::desc("AI Relax"), cl::Hidden);

namespace {
  struct AITolerate : public ModulePass {
    static char ID;
    AITolerate() : ModulePass(ID) {}

    InstrumentFunctionMonitor iFMonitor;
    Function *AITolerateMemoryAccess;
    map<AI_ACCESS_ID, AI_ACCESS_ID> IDMap;

    set<AI_ACCESS_ID> blackList, whiteList;

    bool runOnModule(Module &M);

    bool instrumentLoadOrStore(Instruction *I);
  }; // struct AITolerate
} // namespace

char AITolerate::ID = 0;
static RegisterPass<AITolerate> X("AITolerate", "AI Tolerate Pass");

bool AITolerate::runOnModule(Module &M) {
  IRBuilder<> IRB(M.getContext());

  // Function *pthreadCreate = M.getFunction("pthread_create");
  // if (pthreadCreate) {
  //   iFMonitor.RegisterFunctionReplacement(M, pthreadCreate, "__ai_wrapper_pthread_create");
  // }

  // Module::FunctionListType &functions = M.getFunctionList();
  // for (Module::FunctionListType::iterator it = functions.begin(), it_end = functions.end(); it != it_end; ++it) {
  //   iFMonitor.Instrument(*it);
  // }
  FILE *whiteListFile = fopen("whitelist.ins", "r");
  if (whiteListFile) {
    AI_ACCESS_ID ID;
    while (fscanf(whiteListFile, "%lu", &ID) != EOF)
      whiteList.insert(ID);
  }

  FILE *f = fopen(AISanitizedBSetFile.c_str(), "r");
  size_t sz;
  assert(fscanf(f, "%zu", &sz) != EOF);
  AI_ACCESS_ID a,b;
  while (sz--) {
    assert(fscanf(f, "%lu%lu", &a, &b) != EOF);
    IDMap[a] = b;
  }
  fclose(f);

  blackList.clear();
  if (AIInsGroupFile != "") {
    FILE *g = fopen(AIInsGroupFile.c_str(), "r");
    int tot;
    while (fscanf(g, "%d", &tot) != EOF) {
      AI_ACCESS_ID ins;
      vector<AI_ACCESS_ID> v;
      while (tot--) {
        assert(fscanf(g, "%lu", &ins) != EOF);
        v.push_back(ins);
      }
      double p, p2;

      assert(fscanf(g, "%lf%lf", &p, &p2) != EOF);
      // fprintf(stderr, "%lf %lf\n", p, (double)AIInsGroupThreshold);
      if (p > AIInsGroupThreshold) {
        // fprintf(stderr, "Bloack!\n");
        blackList.insert(v.begin(), v.end());
      }
    }
  }

  bool is_changed = false;
  if (AIRelax == 0) {
    AITolerateMemoryAccess = checkInterfaceFunction(M.getOrInsertFunction("__ai_tolerate_memory_access", IRB.getVoidTy(), 
      IRB.getInt64Ty(), IRB.getInt8PtrTy(), NULL));
  } else {
    AITolerateMemoryAccess = checkInterfaceFunction(M.getOrInsertFunction("__ai_relax_memory_access", IRB.getVoidTy(), 
      IRB.getInt64Ty(), IRB.getInt8PtrTy(), NULL));
  }
  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    Function &F =*MI;
    for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
      BasicBlock &BB = *FI;
      for (BasicBlock::iterator BI = BB.begin(), BE = BB.end(); BI != BE; ++BI) {
        if (isa<LoadInst>(BI) || isa<StoreInst>(BI))
          is_changed |= instrumentLoadOrStore(BI);
      }
    }
  }

  if (is_changed) {
    iFMonitor.addToGlobalCtors(M, "__ai_tolerate_init", IRB.getVoidTy(), NULL);
    iFMonitor.addToGlobalDtors(M, "__ai_tolerate_destroy", IRB.getVoidTy(), NULL);
  }
  return is_changed;
}

bool AITolerate::instrumentLoadOrStore(Instruction *I) {
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
  
  AI_ACCESS_ID id = CI->getZExtValue();
  if (IDMap.count(id) == 0) return false;
  if (blackList.count(id) != 0 && whiteList.count(id) == 0) return false;
  AI_ACCESS_ID _id = IDMap[id];
  
  // Add a Call Before it
  bool IsWrite = isa<StoreInst>(*I);
  Value *Addr = IsWrite
      ? cast<StoreInst>(I)->getPointerOperand()
      : cast<LoadInst>(I)->getPointerOperand();
  
  IRB.CreateCall2(AITolerateMemoryAccess, 
    ConstantInt::get(IRB.getInt64Ty(), _id),
    IRB.CreatePointerCast(Addr, IRB.getInt8PtrTy()));
  
  return true;
}