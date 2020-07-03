#ifndef KERNELREPLACER_HPP
#define KERNELREPLACER_HPP

#include "common.h"

class KernelReplacer : public MatchFinder::MatchCallback {
  using repl_map_t = std::map<std::string, clang::tooling::Replacements>;
public :
  explicit KernelReplacer(repl_map_t& repls) : repls_(repls) {}


  auto kernelMatcher()
  {
    return
    functionDecl(isExpansionInMainFile(),
      hasName("loop_kernel"),
      hasBody(compoundStmt().bind("kernel_compound"))
    ).bind("loop_kernel");
  }  // matcher


  void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("loop_kernel");
    if(!FD){
        llvm::outs()<<"!!!can not find loop_kernel\n";
        return;
    }
    auto path = Context->getSourceManager().getFilename(FD->getLocation()).str();
    auto idx = path.find_last_of('/');
    auto filename = path.substr(idx+1,-1);
    // llvm::outs()<<"filename: "<<filename<<"\n";
    if(std::strcmp(slaveFile.c_str(), filename.c_str())!=0) return;
    // const VarDecl *VD = Result.Nodes.getNodeAs<VarDecl>("var");
  
    // replace prepare-function name 
    // llvm::outs()<< FD->getNameInfo().getName().getAsString() << "\n";
    SourceManager &sm(Context->getSourceManager());
    uint32_t const length =
      sm.getFileOffset(end_of_the_end(FD->getEndLoc(),sm)) - sm.getFileOffset(FD->getBeginLoc());
    string funcBody;
    funcBody += generateExtraHeader();
    for(int i=0;i<functionInfos.size();i++)
    {
        funcBody += "void "+functionInfos[i].funcName + generateKernelFuncDecl(i);
        funcBody += "\n{\n";
        funcBody += "\t"+functionInfos[i].forStmtStr+"}\n";
        funcBody += "}\n\n";
    }
    Replacement replFuncBody(Context->getSourceManager(), FD->getBeginLoc(), length, funcBody);

    // clang::Rewriter rewriter;
    // std::error_code ec;
    // // llvm::raw_fd_ostream outfile{llvm::StringRef(filename), ec, llvm::sys::fs::F_Text };
    // llvm::raw_fd_ostream outFile{"output.txt", ec, llvm::sys::fs::F_Text };
    // applyAllReplacements(repls_, rewriter);
    // rewriter.getEditBuffer(Context->getSourceManager().getMainFileID()).write(outFile);
    // outFile.close();
    // rewriter.setSourceMgr(Result.Context->getSourceManager(),Result.Context->getLangOpts());
    // const ForStmt *FS = Result.Nodes.getNodeAs<ForStmt>("forStmt");
    // FS->dump();

    // {
    if(repls_[filename].add(replFuncBody)) {
      llvm::outs() << "Failed to enter replacement to map for file "
                << filename << "\n";
    }
  };

  string generateExtraHeader() {
    vector<string> strList;
    for (int i = 0; i < functionInfos.size(); ++i)
    {
        bool flag = true;
        for (int j = 0; j < strList.size(); ++j)
        {
            if(functionInfos[i].topoName==strList[j])
            {
                flag = false;
                break;
            }
        }
        if(flag) strList.push_back(functionInfos[i].topoName);
    }
    string stmt;
    for (int i = 0; i < strList.size(); ++i)
    {
        stmt += "label** "+strList[i]+";\n";
    }
    return stmt;
  }
  string generateKernelFuncDecl(int fi) {
    string stmt = "_kernel(";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += "scalar** "+functionInfos[fi].varList[i]+", ";
    }
    stmt += "label "+functionInfos[fi].sizeName+", \n\t";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += "label dim_"+functionInfos[fi].varList[i]+", ";
    }
    stmt += "\n\t";
    for (int i = 0; i < functionInfos[fi].constVarList.size(); ++i)
    {
        stmt += functionInfos[fi].constTypeList[i]+" "+functionInfos[fi].constVarList[i]+", ";
    }
    for (int i = 0; i < functionInfos[fi].arrayNameList.size(); ++i)
    {
        stmt += functionInfos[fi].arrayTypeList[i]+" "+functionInfos[fi].arrayNameList[i]+", ";
    }
    stmt += "\n\t";
    stmt += "label *owner, label *neighbor)";
    return stmt;
  };

private:
  repl_map_t & repls_;
};


class TopoReplacer : public MatchFinder::MatchCallback {
  using repl_map_t = std::map<std::string, clang::tooling::Replacements>;
public :
  explicit TopoReplacer(repl_map_t& repls) : repls_(repls) {}


  auto topoMatcher()
  {
    return varDecl(isExpansionInMainFile(),
        hasDescendant(
        declRefExpr(
        to(varDecl().bind("topoName")))),
        hasDescendant(integerLiteral().bind("sub"))).bind("get_topo");
  }  // matcher


  void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    const VarDecl *VD = Result.Nodes.getNodeAs<VarDecl>("get_topo");
    const VarDecl *VD2 = Result.Nodes.getNodeAs<VarDecl>("topoName");
    if(!VD){
        llvm::outs()<<"!!!can not find get_topo\n";
        return;
    }

    auto path = Context->getSourceManager().getFilename(VD->getLocation()).str();
    auto idx = path.find_last_of('/');
    auto filename = path.substr(idx+1,-1);
    // if(std::strcmp(slaveFile.c_str(), filename.c_str())!=0) return;

    // VD->dump();
    const std::string var = VD->getQualifiedNameAsString();
    const std::string type = VD->getType().getAsString();
    const std::string topoName = VD2->getQualifiedNameAsString();
    bool flag = false;
    for (int i = 0; i < functionInfos.size(); ++i)
    {
        if(functionInfos[i].topoName==topoName) flag = true;
    }
    if(!flag) return;
    // this->varList_.push_back(var);
    // this->typeList_.push_back(VD->getType().getAsString());
    // llvm::outs()<<"var: "<<var<<"\n";
    // llvm::outs()<<VD->getType().getAsString()<<"\n";
    
    const IntegerLiteral *IL = Result.Nodes.getNodeAs<IntegerLiteral>("sub");
    if(!IL){
        llvm::outs()<<"!!!can not find sub\n";
        return;
    }
    // llvm::outs()<<IL->getValue()<<"\n";
    string topoStmt;
    if(IL->getValue()==0) topoStmt = generateTopo(type, var, "owner");
    else if(IL->getValue()==1) topoStmt = generateTopo(type, var, "neighbor");
    else llvm::outs()<<"!!!error\n";

    SourceManager &sm(Context->getSourceManager());
    SourceRange decl_range(VD->getSourceRange());
    SourceLocation decl_begin(decl_range.getBegin());
    SourceLocation decl_start_end(decl_range.getEnd());
    SourceLocation decl_end_end( end_of_the_end(decl_start_end,sm));
    // llvm::outs()<<decl_begin.printToString(sm)<<"\n";
    // llvm::outs()<<VD->getLocation().printToString(sm)<<"\n";

    uint32_t const decl_length =
      sm.getFileOffset(decl_end_end) - sm.getFileOffset(decl_begin);
    // llvm::outs()<<decl_length<<"\n";
    Replacement replTopo(sm,decl_begin,decl_length,topoStmt);

    if(repls_[filename].add(replTopo)) {
      llvm::outs() << "Failed to enter replacement to map for file "
                << filename << "\n";
    }
  };

  string generateTopo(string type, string var, string topo) {
    string stmt = "";
    stmt += type+" ";
    stmt += var+" = ";
    stmt += topo+"[i]";
    return stmt;
  };

private:
  repl_map_t & repls_;
  vector<string> varList_;
  vector<string> typeList_;
  vector<string> topoList_;
};

class SubscriReplacer : public MatchFinder::MatchCallback {
  using repl_map_t = std::map<std::string, clang::tooling::Replacements>;
public :
  explicit SubscriReplacer(repl_map_t& repls) : repls_(repls) {}


  auto subscriMatcher()
  {
    return arraySubscriptExpr(
        hasIndex(integerLiteral().bind("index2")),
        hasDescendant(
            arraySubscriptExpr(
            hasIndex(declRefExpr().bind("index1")),
            hasBase(declRefExpr().bind("field")))));
  }  // matcher


  void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    const DeclRefExpr *DRE = Result.Nodes.getNodeAs<DeclRefExpr>("field");
    if(!DRE){
        llvm::outs()<<"!!!can not find field\n";
        return;
    }
    auto path = Context->getSourceManager().getFilename(DRE->getLocation()).str();
    auto idx = path.find_last_of('/');
    auto filename = path.substr(idx+1,-1);
    // if(std::strcmp(slaveFile.c_str(), filename.c_str())!=0) return;

    // VD->dump();
    const std::string var = DRE->getNameInfo().getName().getAsString();
    bool flag = false;
    for (int i = 0; i < functionInfos.size(); ++i)
    {
        for (int j = 0; j < functionInfos[i].varList.size(); ++j)
        {
            if(var==functionInfos[i].varList[j]) flag = true;
        }
    }
    if(!flag) return;
    // this->varList_.push_back(var);
    // this->typeList_.push_back(VD->getType().getAsString());
    // llvm::outs()<<"var: "<<var<<"\n";
    // llvm::outs()<<VD->getType().getAsString()<<"\n";
    
    const IntegerLiteral *IL = Result.Nodes.getNodeAs<IntegerLiteral>("index2");
    const DeclRefExpr *DRE1 = Result.Nodes.getNodeAs<DeclRefExpr>("index1");
    if(!IL || !DRE1){
        llvm::outs()<<"!!!can not find index1\n";
        return;
    }
    // llvm::outs()<<IL->getValue()<<"\n";
    const string index1 = DRE1->getNameInfo().getName().getAsString();
    int index2 = IL->getValue().getLimitedValue();
    // llvm::outs()<<DRE1->getNameInfo().getName().getAsString()<<"\n";
    string newSubscri = index1+"*dim_"+var+"+"+to_string(index2);

    // 获取长度
    SourceManager &sm(Context->getSourceManager());
    uint32_t const index1_length =
      sm.getFileOffset(end_of_the_end(DRE1->getEndLoc(),sm)) - sm.getFileOffset(DRE1->getBeginLoc());
    uint32_t const index2_length =
      sm.getFileOffset(end_of_the_end(IL->getEndLoc(),sm)) - sm.getFileOffset(IL->getBeginLoc());
    // llvm::outs()<<index1_length<<"\n";
    // llvm::outs()<<index2_length<<"\n";

    Replacement replSubscri(Context->getSourceManager(),
        DRE1->getBeginLoc(),
        index1_length+index2_length+2,
        newSubscri);

    if(repls_[filename].add(replSubscri)) {
      llvm::outs() << "Failed to enter replacement to map for file "
                << filename << "\n";
    }
};

private:
  repl_map_t & repls_;
};

class PointerReplacer : public MatchFinder::MatchCallback {
  using repl_map_t = std::map<std::string, clang::tooling::Replacements>;
public :
  explicit PointerReplacer(repl_map_t& repls) : repls_(repls) {}


  auto pointerMatcher()
  {
    return parmVarDecl(
        isExpansionInMainFile(),
        hasAncestor(functionDecl(matchesName("kernel")).bind("func")),
        hasType(isAnyPointer())).bind("parm");
  }  // matcher


  void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    const ParmVarDecl *PVD = Result.Nodes.getNodeAs<ParmVarDecl>("parm");
    const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func");
    if(!PVD || !FD){
        llvm::outs()<<"!!!can not find field\n";
        return;
    }
    auto path = Context->getSourceManager().getFilename(PVD->getLocation()).str();
    auto idx = path.find_last_of('/');
    auto filename = path.substr(idx+1,-1);

    const string var = PVD->getQualifiedNameAsString();
    bool flag = false;
    for (int i = 0; i < functionInfos.size(); ++i)
    {
        for (int j = 0; j < functionInfos[i].varList.size(); ++j)
        {
            if(var==functionInfos[i].varList[j]) flag = true;
        }
    }
    if(!flag) return;

    SourceManager &sm(Context->getSourceManager());
    uint32_t parm_begin = sm.getFileOffset(PVD->getBeginLoc());
    uint32_t parm_end   = sm.getFileOffset(PVD->getLocation());
    // llvm::outs()<<parm_end-parm_begin<<"\n";
    // llvm::outs()<<PVD->getBeginLoc().printToString(sm)<<"\n";
    // llvm::outs()<<PVD->getBeginLoc().printToString(sm)<<"\n";
    // llvm::outs()<<PVD->getLocation().printToString(sm)<<"\n";
    Replacement replPointer(Context->getSourceManager(),
        PVD->getBeginLoc(),
        parm_end-parm_begin,
        "scalar* ");

    if(repls_[filename].add(replPointer)) {
      llvm::outs() << "Failed to enter replacement to map for file "
                << filename << "\n";
    }
  }

private:
  repl_map_t & repls_;
};

#endif