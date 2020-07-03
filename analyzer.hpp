#ifndef ANALYZER_HPP
#define ANALYZER_HPP

#include "common.h"

class LoopPrinter : public MatchFinder::MatchCallback {
public :

auto forStmtMatcher(){
    return
    forStmt(isExpansionInMainFile(), hasAncestor(functionDecl().bind("func"))).bind("forLoop");
}  // matcher

void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;
  const ForStmt *FS = Result.Nodes.getNodeAs<ForStmt>("forLoop");
  const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  if(!FS || !FD){
      llvm::outs()<<"!!!can not find forLoop\n";
      return;
  }
  // auto path = Context->getSourceManager().getFilename(FS->getBeginLoc()).str();
  // auto idx = path.find_last_of('/');
  // auto filename = path.substr(idx+1,-1);
  llvm::outs()<<"funcName: "<<FD->getNameInfo().getName().getAsString()<<"\n";
  // if(std::strcmp(srcFile.c_str(), filename.c_str())!=0) return;

  const Stmt *funcBody = FS->getBody();
  clang::SourceRange loc = funcBody->getSourceRange();
  auto locStart = Context->getSourceManager().getPresumedLoc(loc.getBegin());
  auto locEnd = Context->getSourceManager().getPresumedLoc(loc.getEnd());
  // llvm::outs()<< locStart.getLine()<< ":" << locEnd.getLine() << "\n";
  // llvm::outs()<< locStart.getColumn() <<":" << locEnd.getColumn() << "\n";
  // funcBody->dump();
  // llvm::StringRef ref = Lexer::getSourceText(CharSourceRange::getCharRange(funcBody->getSourceRange()), Context->getSourceManager(), Context->getLangOpts());
  llvm::StringRef header = Lexer::getSourceText(CharSourceRange::getCharRange(FS->getSourceRange()), Context->getSourceManager(), Context->getLangOpts());
  // forStmtStr = header.str();
  // llvm::outs()<<header.str()<<"\n";

  const string name = FD->getNameInfo().getName().getAsString();
  for (int i = 0; i < functionInfos.size(); ++i)
  {
      if(functionInfos[i].funcName==name)
        functionInfos[i].forStmtStr = header.str();
  }
};
};

class FuncPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;

  const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  if(!FD){
      llvm::outs()<<"!!!can not find func\n";
      return;
  }
  auto path = Context->getSourceManager().getFilename(FD->getLocation()).str();
  auto idx = path.find_last_of('/');
  auto filename = path.substr(idx+1,-1);
  // llvm::outs()<<"filename: "<<filename<<"\n";
  // if(std::strcmp(srcFile.c_str(), filename.c_str())!=0) return;

  // llvm::outs() << "Potential functions discovered " << FD->getNameInfo().getName().getAsString() << "\n";
  string funcName = FD->getNameInfo().getName().getAsString();
  FunctionInfo funcInfo(funcName);
  functionInfos.push_back(funcInfo);

/////////
    // llvm::outs() << "FunctionDecl@:"<<FD<<":"
    //     << FD->getReturnType().getAsString()<<" "
    //     << FD->getQualifiedNameAsString()
    //     <<"(";

    // for (int i = 0; i < FD->getNumParams(); i++)
    // {
    //     if (i > 0) llvm::outs() << ",";             
    //     llvm::outs() 
    //         << QualType::getAsString(FD->parameters()[i]->getType().split()
    //             , PrintingPolicy{ {} })<<" "
    //         << FD->parameters()[i]->getQualifiedNameAsString();          
    // }

    // llvm::outs() << ")"
    //     <<"   Definition@"<<FD->getDefinition()
    //     <<"\n";

};
};

class VarPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;

  const VarDecl *VD = Result.Nodes.getNodeAs<VarDecl>("var");
  const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  if(!VD || !FD){
      llvm::outs()<<"!!!can not find var\n";
      return;
  }
  // VD->dump();
  auto path = Context->getSourceManager().getFilename(VD->getLocation()).str();
  auto idx = path.find_last_of('/');
  auto filename = path.substr(idx+1,-1);
  // llvm::outs()<<"filename: "<<filename<<"\n";
  // if(std::strcmp(srcFile.c_str(), filename.c_str())!=0) return;

  const std::string var = VD->getQualifiedNameAsString();
  // const std::string setType
    // = Result.Nodes.getNodeAs<clang::StringLiteral>("setType")->getString().str();
  const std::string fieldName
    = Result.Nodes.getNodeAs<DeclRefExpr>("parm")
    ->getNameInfo().getName().getAsString();
  // llvm::outs()<<"var: "<<var<<", ";
  // llvm::outs()<<"setType: "<<setType<<", ";
  // llvm::outs()<<"fieldName: "<<fieldName<<"\n";
  // varList.push_back(var);
  // fieldNameList.push_back(fieldName);
  // inoutList.resize(varList.size());
  const string name = FD->getNameInfo().getName().getAsString();
  for (int i = 0; i < functionInfos.size(); ++i)
  {
      if(functionInfos[i].funcName==name)
      {
        functionInfos[i].varList.push_back(var);
        functionInfos[i].fieldNameList.push_back(fieldName);
        functionInfos[i].inoutList.resize(functionInfos[i].varList.size());
      }
  }

};
};

class TopoPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;

  const VarDecl *VD = Result.Nodes.getNodeAs<VarDecl>("topo");
  const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  if(!VD || !FD){
      llvm::outs()<<"!!!can not find topo\n";
      return;
  }
  auto path = Context->getSourceManager().getFilename(VD->getLocation()).str();
  auto idx = path.find_last_of('/');
  auto filename = path.substr(idx+1,-1);
  // llvm::outs()<<"filename: "<<filename<<"\n";
  // if(std::strcmp(srcFile.c_str(), filename.c_str())!=0) return;

  // topoName = VD->getQualifiedNameAsString();

  const string name = FD->getNameInfo().getName().getAsString();
  for (int i = 0; i < functionInfos.size(); ++i)
  {
      if(functionInfos[i].funcName==name)
        functionInfos[i].topoName = VD->getQualifiedNameAsString();
  }
};
};

class ConstPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;

  const ParmVarDecl *PVD = Result.Nodes.getNodeAs<ParmVarDecl>("const");
  const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  if(!PVD || !FD){
      llvm::outs()<<"!!!can not find const\n";
      return;
  }

  PrintingPolicy pp(Context->getLangOpts());
  const string var = PVD->getQualifiedNameAsString();
  const string type = PVD->getType().getUnqualifiedType().getCanonicalType().getAsString(pp);
  // constVarList.push_back(var);
  // constTypeList.push_back(type);
  // llvm::outs()<<var<<"\n";
  // llvm::outs()<<type<<"\n";
  const string name = FD->getNameInfo().getName().getAsString();
  for (int i = 0; i < functionInfos.size(); ++i)
  {
      if(functionInfos[i].funcName==name)
      {
        functionInfos[i].constVarList.push_back(var);
        functionInfos[i].constTypeList.push_back(type);
      }
  }
};
};

class ArrayPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;

  const ParmVarDecl *PVD = Result.Nodes.getNodeAs<ParmVarDecl>("array");
  const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  if(!PVD || !FD){
      llvm::outs()<<"!!!can not find array\n";
      return;
  }

  PrintingPolicy pp(Context->getLangOpts());
  const string var = PVD->getQualifiedNameAsString();
  const string type = PVD->getType().getUnqualifiedType().getCanonicalType().getAsString(pp);
  // llvm::outs()<<PVD->getType().getAsString()<<"\n";
  // llvm::outs()<<PVD->getType().getUnqualifiedType().getAsString()<<"\n";
  // llvm::outs()<<PVD->getType().getUnqualifiedType().getCanonicalType().getAsString()<<"\n";
  SourceManager &sm(Context->getSourceManager());
  const string name = FD->getNameInfo().getName().getAsString();
  uint32_t const func_begin = sm.getFileOffset(FD->getBeginLoc());
  uint32_t const func_end   = sm.getFileOffset(end_of_the_end(FD->getEndLoc(),sm));
  // llvm::outs()<<"func_begin"<<func_begin<<"\n";
  // llvm::outs()<<"func_end"<<func_end<<"\n";
  for (int i = 0; i < functionInfos.size(); ++i)
  {
    // 首先找到该变量位于哪个函数体内
    if(functionInfos[i].funcName==name)
    {
      functionInfos[i].arrayNameList.push_back(var);
      functionInfos[i].arrayTypeList.push_back(type);
      // 遍历pragma存储的数组，确定变量的维度
      bool flag = false;
      for (int j = 0; j < arrayNameList.size(); ++j)
      {
  // llvm::outs()<<"arrayLocList"<<arrayLocList[j]<<"\n";
        // 如果变量名与pragma声明的变量名相同，并且pragma位置位于该函数体内，
        // 则获取它的维度
        if(var==arrayNameList[j] && arrayLocList[j] > func_begin && arrayLocList[j] < func_end) {
            flag = true;
            functionInfos[i].arraySizeList.push_back(arraySizeList[j]);
        }
      }
      if(!flag){
        llvm::outs()<<"The size of array variable has not been assigned through pragma.\n";
      return;
      }
    }
  }
  // bool flag = false;
  // for (int i = 0; i < arrayNameList.size(); ++i)
  // {
  //     if(var==arrayNameList[i])
  //     {
  //       arrayTypeList[i] = type;
  //       flag = true;
  //     }
  // }
  // if(!flag){
  //   llvm::outs()<<"The size of array variable has not been assigned through pragma.\n";
  //   return;
  // }
  // llvm::outs()<<var<<"\n";
  // llvm::outs()<<type<<"\n";
};
};

class InoutPrinter : public MatchFinder::MatchCallback {
public :
void run(const MatchFinder::MatchResult &Result) {
  ASTContext *Context = Result.Context;

  const BinaryOperator *BO = Result.Nodes.getNodeAs<BinaryOperator>("binary");
  const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
  if(!BO || !FD){
      llvm::outs()<<"!!!can not find binary\n";
      return;
  }
  auto path = Context->getSourceManager().getFilename(BO->getExprLoc()).str();
  auto idx = path.find_last_of('/');
  auto filename = path.substr(idx+1,-1);
  // llvm::outs()<<"filename: "<<filename<<"\n";
  // if(std::strcmp(srcFile.c_str(), filename.c_str())!=0) return;
  // if(std::strcmp(srcFile.c_str(), filename.c_str())!=0) return;

  const unsigned int binaryLoc = BO->getOperatorLoc().getRawEncoding();
  const std::string binaryOp = BO->getOpcodeStr().str();
  // llvm::outs()<<binaryOp<<"\n";
  // llvm::outs()<<BO->getOperatorLoc().getRawEncoding()<<"\n";
  const DeclRefExpr *DRE = Result.Nodes.getNodeAs<DeclRefExpr>("declRefExpr");
  const std::string name = DRE->getNameInfo().getName().getAsString();
  // for (int i = 0; i < varList.size(); ++i)
  // {
  //   if(std::strcmp(varList[i].c_str(), name.c_str())!=0) continue;
  //   const unsigned int varLoc = DRE->getLocation().getRawEncoding();
  //   if(varLoc<binaryLoc)
  //   {
  //     if(binaryOp=="=" && inoutList[i]!=3) inoutList[i] = 2; //copyout
  //     else inoutList[i] = 3; // copyinout
  //   }
  //   else inoutList[i] = 1;
  //   // llvm::outs()<<"name: "<<name<<"\n";
  //   // DRE->dump();
  // }
  const string funname = FD->getNameInfo().getName().getAsString();
  for (int i = 0; i < functionInfos.size(); ++i)
  {
// llvm::outs()<<i<<", "<<functionInfos[i].funcName<<", "<<funname<<"\n";
      if(functionInfos[i].funcName==funname)
      {
        for (int j = 0; j < functionInfos[i].varList.size(); ++j)
        {
          if(std::strcmp(functionInfos[i].varList[j].c_str(), name.c_str())!=0) continue;
          const unsigned int varLoc = DRE->getLocation().getRawEncoding();
          if(varLoc<binaryLoc)
          {
            if(binaryOp=="=" && functionInfos[i].inoutList[j]!=3)
                functionInfos[i].inoutList[j] = 2; //copyout
            else
                functionInfos[i].inoutList[j] = 3; // copyinout
          }
          else
            functionInfos[i].inoutList[j] = 1;
        }
      }
  }
};
};


#endif