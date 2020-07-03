#ifndef COMMON_H
#define COMMON_H

// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/AST/ASTContext.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

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
#include <fstream>

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;

// string funcName;
// vector<string> varList;
// vector<string> setTypeList;
// vector<string> fieldNameList;
// vector<unsigned int> inoutList; 
// vector<string> constVarList; // 常数变量名
// vector<string> constTypeList; // 常数类型
vector<string> arrayNameList; // 常数数组名
vector<string> arrayTypeList; // 常数数组类型
vector<unsigned int> arraySizeList; // 常数数组长度
vector<unsigned int> arrayLocList; // 常数数组长度

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

DeclarationMatcher getFieldMatcher =
    varDecl(isExpansionInMainFile(),
        hasAncestor(functionDecl().bind("func")),
        has(cxxMemberCallExpr(
        has(memberExpr(member(hasName("getField")))),
        hasArgument(0, declRefExpr().bind("parm"))))).bind("var");

DeclarationMatcher getTopoMatcher =
    varDecl(isExpansionInMainFile(),
        hasAncestor(functionDecl().bind("func")),
        has(cxxMemberCallExpr(
        has(memberExpr(member(hasName("getTopology"))))))).bind("topo");

DeclarationMatcher getSizeMatcher =
    varDecl(isExpansionInMainFile(),
        hasAncestor(functionDecl().bind("func")),
        has(cxxMemberCallExpr(
        has(memberExpr(member(hasName("getSize"))))))).bind("size");


StatementMatcher InoutMatcher =
    declRefExpr(isExpansionInMainFile(),
        hasAncestor(functionDecl().bind("func")),
        hasAncestor(binaryOperator(hasAncestor(forStmt()),isAssignmentOperator()).bind("binary"))).bind("declRefExpr");

DeclarationMatcher constMatcher =
    parmVarDecl(isExpansionInMainFile(),
        hasAncestor(functionDecl().bind("func")),
        hasType(isConstQualified()),
        unless(hasType(isAnyPointer()))).bind("const");

DeclarationMatcher arrayMatcher =
    parmVarDecl(isExpansionInMainFile(),
        hasAncestor(functionDecl().bind("func")),
        hasType(isAnyPointer())).bind("array");

// std::string srcFile = "funPtr_host.cpp";
std::string objFile = "kernel_pre.cpp";
std::string slaveFile = "kernel_slave.c";

// forStmt(isExpansionInMainFile(),hasAncestor(functionDecl()))
std::string forStmtStr;
std::string topoName;

void initSlaveFile();
void initPreFile();

SourceLocation end_of_the_end(SourceLocation const & start_of_end, SourceManager & sm){
  LangOptions lopt;
  return Lexer::getLocForEndOfToken(start_of_end, 0, sm, lopt);
}

class FunctionInfo
{
public:
  explicit FunctionInfo(string& funcName) : funcName(funcName) {}
  string funcName; // 函数名
  vector<string> varList; // field场变量名
  vector<string> setTypeList; // field场数据集类型（暂时不用）
  vector<string> fieldNameList; // field场名字
  vector<unsigned int> inoutList; // field场输入输出类型
  vector<string> constVarList; // 常数变量名
  vector<string> constTypeList; // 常数类型
  vector<string> arrayNameList; // 常数数组名
  vector<string> arrayTypeList; // 常数数组类型
  vector<unsigned int> arraySizeList; // 常数数组长度
  std::string forStmtStr; // for循环体字符串
  std::string topoName; // 拓扑变量名
  std::string sizeName; // 维度变量名

  void print()
  {
    llvm::outs()<<"Function: "<<funcName<<"\n";
    llvm::outs()<<"Const Parameter: ";
    for (int i = 0; i < constVarList.size(); ++i)
    {
        llvm::outs()<<constTypeList[i]<<" "<<constVarList[i]<<", ";
    }
    for (int i = 0; i < arrayNameList.size(); ++i)
    {
        llvm::outs()<<arrayTypeList[i]<<" "<<arrayNameList[i]<<"["<<arraySizeList[i]<<"]"<<",";
    }
    llvm::outs()<<"\nField Parameter: ";
    for (int i = 0; i < varList.size(); ++i)
    {
        llvm::outs()<<varList[i]<<"("<<fieldNameList[i]<<")"<<"("<<inoutList[i]<<"), ";
    }
    llvm::outs()<<"\nTopology variable name: "<<topoName<<"\n";
    llvm::outs()<<"\nTopology size name: "<<sizeName<<"\n";
    llvm::outs()<<"For loop: \n";
    llvm::outs()<<forStmtStr<<"\n";
  }
};

vector<FunctionInfo> functionInfos;


#endif