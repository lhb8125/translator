// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/AST/ASTContext.h"
#include "clang/Tooling/Refactoring.h"

using namespace clang;
using namespace clang::tooling;
//using namespace clang::StringLiteral;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;

vector<string> varList;
vector<string> setTypeList;
vector<string> fieldNameList;
vector<unsigned int> inoutList;

StatementMatcher LoopMatcher =
    forStmt(isExpansionInMainFile(),
			hasLoopInit(declStmt(
                hasSingleDecl(varDecl(hasInitializer(integerLiteral(equals(0))))
                                  .bind("initVarName")))),
            hasIncrement(unaryOperator(
                hasOperatorName("++"),
                hasUnaryOperand(declRefExpr(
                    to(varDecl(hasType(isInteger())).bind("incVarName")))))),
            hasCondition(binaryOperator(
                hasOperatorName("<"),
                hasLHS(ignoringParenImpCasts(declRefExpr(
                    to(varDecl(hasType(isInteger())).bind("condVarName"))))),
                hasRHS(expr(hasType(isInteger())))))).bind("forLoop");

DeclarationMatcher FuncMatcher =
    functionDecl(isExpansionInMainFile()).bind("func");

DeclarationMatcher VarMatcher =
    varDecl(isExpansionInMainFile(),
	has(cxxMemberCallExpr(
	hasArgument(0, stringLiteral().bind("setType")),
	hasArgument(1, stringLiteral().bind("fieldName"))))).bind("var");

StatementMatcher InoutMatcher =
	declRefExpr(isExpansionInMainFile(),hasAncestor(binaryOperator(hasAncestor(forStmt()),isAssignmentOperator()).bind("binary"))).bind("declRefExpr");
//StatementMatcher InoutMatcher =
//	declRefExpr(isExpansionInMainFile()).bind("inout");

class LoopPrinter : public MatchFinder::MatchCallback {
public :
static bool areSameVariable(const ValueDecl *First, const ValueDecl *Second) {
  return First && Second &&
         First->getCanonicalDecl() == Second->getCanonicalDecl();
};
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;
  const ForStmt *FS = Result.Nodes.getNodeAs<ForStmt>("forLoop");
  // We do not want to convert header files!
  if (!FS || !Context->getSourceManager().isWrittenInMainFile(FS->getForLoc()))
    return;
  const VarDecl *IncVar = Result.Nodes.getNodeAs<VarDecl>("incVarName");
  const VarDecl *CondVar = Result.Nodes.getNodeAs<VarDecl>("condVarName");
  const VarDecl *InitVar = Result.Nodes.getNodeAs<VarDecl>("initVarName");

  if (!areSameVariable(IncVar, CondVar) || !areSameVariable(IncVar, InitVar))
    return;
  llvm::outs() << "Potential array-based loop discovered.\n";
};
};

class FuncPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;
  const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  llvm::outs() << "Potential functions discovered " << FD->getNameInfo().getName().getAsString() << "\n";
};
};

class VarPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;
  const VarDecl *VD = Result.Nodes.getNodeAs<VarDecl>("var");
  const std::string var = VD->getQualifiedNameAsString();
  const std::string setType = Result.Nodes.getNodeAs<clang::StringLiteral>("setType")->getString().str();
  const std::string fieldName = Result.Nodes.getNodeAs<clang::StringLiteral>("fieldName")->getString().str();
  // llvm::outs()<<"var: "<<var<<", ";
  // llvm::outs()<<"setType: "<<setType<<", ";
  // llvm::outs()<<"fieldName: "<<fieldName<<"\n";
  varList.push_back(var);
  setTypeList.push_back(setType);
  fieldNameList.push_back(fieldName);
  inoutList.resize(varList.size());

};
};

class InoutPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;
  const BinaryOperator *BO = Result.Nodes.getNodeAs<BinaryOperator>("binary");
  const unsigned int binaryLoc = BO->getOperatorLoc().getRawEncoding();
  const std::string binaryOp = BO->getOpcodeStr().str();
  // llvm::outs()<<binaryOp<<"\n";
  // llvm::outs()<<BO->getOperatorLoc().getRawEncoding()<<"\n";
  const DeclRefExpr *DRE = Result.Nodes.getNodeAs<DeclRefExpr>("declRefExpr");
  const std::string name = DRE->getNameInfo().getName().getAsString();
  for (int i = 0; i < varList.size(); ++i)
  {
    if(std::strcmp(varList[i].c_str(), name.c_str())!=0) continue;
    const unsigned int varLoc = DRE->getLocation().getRawEncoding();
    if(varLoc<binaryLoc)
    {
      if(binaryOp=="=" && inoutList[i]!=3) inoutList[i] = 2; //copyout
      else inoutList[i] = 3; // copyinout
    }
    else inoutList[i] = 1;
    // llvm::outs()<<"name: "<<name<<"\n";
    // DRE->dump();
  }
};
};

class Replacer : public MatchFinder::MatchCallback {
  using repl_map_t = std::map<std::string, clang::tooling::Replacements>;
public :
  explicit Replacer(repl_map_t& repls) : repls_(repls) {}


  void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    const VarDecl *VD = Result.Nodes.getNodeAs<VarDecl>("var");
  
    // insert
    Replacement repl(Context->getSourceManager(), VD->getLocation(), 6, "hello world");
    // now add to the replacements for the source file in which this
    // declaration was found
    auto filename = Context->getSourceManager().getFilename(VD->getLocation()).str();
    std::string myname = "/home/export/online1/amd_dev1/liuhb/hsf/src/iterator/funPtr_pre_slave.cpp";
    // Replacements repls;
    // repls.add(repl);
    // repls_.insert(pair<std::string, Replacements>(filename, repls));
    if(std::strcmp(myname.c_str(), filename.c_str())==0)
    {
      if(repls_[filename].add(repl)) {
        llvm::outs() << "Failed to enter replacement to map for file "
                  << filename << "\n";
      }
    } else
      llvm::outs() << "wrong file: "<< filename << "\n";
  };
private:
  repl_map_t & repls_;
};

int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

//  LoopPrinter Printer;
//  MatchFinder Finder;
//  Finder.addMatcher(LoopMatcher_test, &Printer);

  FuncPrinter funcPrinter;
  RefactoringTool refactTool(OptionsParser.getCompilations(),
                             OptionsParser.getSourcePathList());
  VarPrinter varPrinter;
  InoutPrinter inoutPrinter;
  Replacer replacer(refactTool.getReplacements());
  MatchFinder Finder;
//  Finder.addMatcher(FuncMatcher, &funcPrinter);
  Finder.addMatcher(VarMatcher, &varPrinter);
  Finder.addMatcher(InoutMatcher, &inoutPrinter);
  Finder.addMatcher(VarMatcher, &replacer);
  Tool.run(newFrontendActionFactory(&Finder).get());
  refactTool.runAndSave(newFrontendActionFactory(&Finder).get());

  for (int i = 0; i < varList.size(); ++i)
  {
    llvm::outs()<<"name: "<<varList[i]<<", ";
    llvm::outs()<<"setType: "<<setTypeList[i]<<", ";
    llvm::outs()<<"fieldName: "<<fieldNameList[i]<<", ";
    llvm::outs()<<"inout: "<<inoutList[i]<<"\n";
  }

  for(auto & p : refactTool.getReplacements()) {
    auto & fname = p.first;
    auto & repls = p.second;
    llvm::outs() << "Replacements collected for file \"" << fname
              << "\" (applied):\n";
    for(auto & r : repls) { llvm::outs() << "\t" << r.toString() << "\n"; }
  }

  return 0;
}
