#include "instrumentation/Function.h"

#include "stdio.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <set>
#include <vector>

#include "common.hpp"


using namespace std;
using namespace llvm;
using namespace llvm_instrumentation;

static cl::opt<std::string>  WhiteListFileName("WhiteListFileName", cl::init(""),
       cl::desc("White List File"), cl::Hidden);

namespace {
  struct AIPrepare : public ModulePass {
    static char ID;
    AIPrepare() : ModulePass(ID), TD(0) {}

    DataLayout *TD;

    bool runOnModule(Module &M);

    uint32_t getMemoryAccessTypeSize(Value *Addr);
    AI_ACCESS_ID instrumentLoadOrStore(Instruction *I);
    int numServer;
    sockaddr_in sin;
    socklen_t sin_len;
  }; // struct AIPrepare
} // namespace

char AIPrepare::ID = 0;
static RegisterPass<AIPrepare> X("AIPrepare", "AI Prepare Pass");

bool AIPrepare::runOnModule(Module &M) {
  bool useWhiteList = (WhiteListFileName.length() != 0);
  FILE* whiteListFile = NULL;
  if (useWhiteList) {
    whiteListFile = fopen(WhiteListFileName.c_str(), "w");
  }

  // Connect to id-server
  bzero(&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(AI_NUM_SERVER_PORT);
  sin_len = sizeof(sin);
  numServer = socket(AF_INET, SOCK_DGRAM, 0);

  TD = getAnalysisIfAvailable<DataLayout>();
  if (!TD)
    report_fatal_error("getAnalysisIfAvailable fail!");

  set<AI_ACCESS_ID> whiteList;
  for (Module::iterator MI = M.begin(), ME = M.end(); MI != ME; ++MI) {
    Function &F =*MI;

    vector<AI_ACCESS_ID> insFunc;
    bool insThisFunc = false;
    for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
      BasicBlock &BB = *FI;
      vector<AI_ACCESS_ID> insBB;
      bool insThisBB = false;
      for (BasicBlock::iterator BI = BB.begin(), BE = BB.end(); BI != BE; ++BI) {
        if (isa<LoadInst>(BI) || isa<StoreInst>(BI)) {
          AI_ACCESS_ID ID = instrumentLoadOrStore(BI);
          if (ID) {
            insFunc.push_back(ID);
            insBB.push_back(ID);
          }
        }
        if (useWhiteList) {
          StringRef fName = "";
          if (InvokeInst *ii = dyn_cast<InvokeInst>(BI)) {
            fName =  ii->getCalledFunction()->getName();
          }
          if (CallInst *ci = dyn_cast<CallInst>(BI)) {
            fName =  ci->getCalledFunction()->getName();
          }
          if (fName.equals("__ai_ins_this_bb"))
            insThisBB = true;
          if (fName.equals("__ai_ins_this_func"))
            insThisFunc = true;
          
        }
      }
      if (insThisBB) {
        whiteList.insert(insBB.begin(), insBB.end());
      }
    }
    if (insThisFunc) {
      whiteList.insert(insFunc.begin(), insFunc.end());
    }
  }

  if (useWhiteList) {
    for (set<AI_ACCESS_ID>::iterator it=whiteList.begin(); it != whiteList.end(); ++it) {
      fprintf(whiteListFile, "%lu\n", *it);
    }
    fclose(whiteListFile);
  }

  return true;
}

uint32_t AIPrepare::getMemoryAccessTypeSize(Value *Addr) {
  Type *OrigPtrTy = Addr->getType();
  Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
  assert(OrigTy->isSized());
  uint32_t TypeSize = TD->getTypeStoreSizeInBits(OrigTy);
  return TypeSize;
}

AI_ACCESS_ID AIPrepare::instrumentLoadOrStore(Instruction *I) {
  IRBuilder<> IRB(I);
  
  bool IsWrite = isa<StoreInst>(*I);
  Value *Addr = IsWrite
      ? cast<StoreInst>(I)->getPointerOperand()
      : cast<LoadInst>(I)->getPointerOperand();
  uint32_t TypeSize = getMemoryAccessTypeSize(Addr);
  unsigned Line = -1; // Unknow
  StringRef File = "", Dir = "";
  if (MDNode *N = I->getMetadata("dbg")) {
    DILocation Loc(N);                    
    Line = Loc.getLineNumber();
    File = Loc.getFilename();
    Dir = Loc.getDirectory();
  }

  if (Line == (unsigned)-1) {
    // HACK: Do not instrument unknow instructions
    return 0;
  }

  char message[AI_NUM_SERVER_MESSAGE_MAX_SIZE];
  sprintf(message, "%d\t|\t%u\t|\t%u\t|\t%s\t|\t%s\n",
    IsWrite, TypeSize, Line, File.data(), Dir.data());
  sendto(numServer, message, sizeof(message), 0, (struct sockaddr *)&sin, sin_len);
  
  AI_ACCESS_ID ID;
  recvfrom(numServer, &ID, sizeof(ID), 0, NULL, NULL);

  // Set AIMemoryAccessID
  ConstantInt *CI = ConstantInt::get(IRB.getInt64Ty(), ID);
  SmallVector<Value*, 1> V;
  V.push_back(CI);
  MDNode *Node = MDNode::get(I->getContext(), V);

  I->setMetadata("AIMemoryAccessID", Node);
  
  // fprintf(LogFile,"%lu\t|\t%d\t|\t%u\t|\t%u\t|\t%s\t|\t%s\n",
  //   numMemoryAccess, IsWrite, TypeSize, Line, File.data(), Dir.data());

  return ID;
}
