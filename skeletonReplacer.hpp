#ifndef SKELETONREPLACER_HPP
#define SKELETONREPLACER_HPP

#include "common.h"


class SkeletonReplacer : public MatchFinder::MatchCallback {
  using repl_map_t = std::map<std::string, clang::tooling::Replacements>;
public :
  explicit SkeletonReplacer(repl_map_t& repls) : repls_(repls) {}


  auto kernelMatcher()
  {
    return
    functionDecl(isExpansionInMainFile(),
      hasName("loop_skeleton"),
      hasBody(compoundStmt().bind("skeleton_compound"))
    ).bind("loop_skeleton");
  }  // matcher


  void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("loop_skeleton");
    if(!FD){
        llvm::outs()<<"!!!can not find loop_skeleton\n";
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
    for(int i=0;i<functionInfos.size();i++)
    {
        funcBody += "void "+functionInfos[i].funcName + generateSkeletonFuncDecl();
        funcBody += "\n{";
        funcBody += generateAccessArray(i);
        funcBody += generateAccessDim(i);
        funcBody += generateAccessConst(i);
        funcBody += generateRest(i);
        funcBody += "}\n\n";
    }
    Replacement replFuncBody(Context->getSourceManager(), FD->getBeginLoc(), length, funcBody);

    // {
    if(repls_[filename].add(replFuncBody)) {
      llvm::outs() << "Failed to enter replacement to map for file "
                << filename << "\n";
    }
  };

  string generateSkeletonFuncDecl() {
    string stmt = "_skeleton(DataSet *dataSet_parm, DataSet *dataSet_edge, ";
    stmt += "DataSet *dataSet_vertex, label *row, label *col)";
    // llvm::outs()<<stmt<<"\n";
    return stmt;
  };

  string generateAccessArray(int fi){
    string stmt = "\n";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += "\tscalar *arr_"+functionInfos[fi].varList[i];
        stmt += " = getArrayFromDataSet(\""+functionInfos[fi].fieldNameList[i]+"\");\n";
    }
    // llvm::outs()<<stmt<<"\n";
    return stmt;
  }

  string generateAccessDim(int fi){
    string stmt = "\n";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += "\tlabel dim_"+functionInfos[fi].varList[i];
        stmt += " = getDimFromDataSet(\""+functionInfos[fi].fieldNameList[i]+"\");\n";
    }
    // llvm::outs()<<stmt<<"\n";
    return stmt;
  }

  string generateAccessConst(int fi){
    string stmt = "\n";
    for (int i = 0; i < functionInfos[fi].constVarList.size(); ++i)
    {
        stmt += "\t"+functionInfos[fi].constTypeList[i]+" const_"+functionInfos[fi].constVarList[i];
        stmt += " = *("+functionInfos[fi].constTypeList[i]+"*)getConstFromDataSet("+std::to_string(i)+");\n";
    }
    for (int i = 0; i < functionInfos[fi].arrayNameList.size(); ++i)
    {
        stmt += "\t"+functionInfos[fi].arrayTypeList[i]+" const_"+functionInfos[fi].arrayNameList[i];
        stmt += " = ("+functionInfos[fi].arrayTypeList[i]+")getConstFromDataSet("
            +std::to_string(i+functionInfos[fi].constVarList.size())+");\n";
    }
    // llvm::outs()<<stmt<<"\n";
    return stmt;
  }

  string generateRest(int fi){
    string stmt = "\n\tlabel n = getSizeFromDataSet(dataSet_edge, dataSet_vertex);\n";
    stmt += "\n\t"+functionInfos[fi].funcName+"_kernel(";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += "arr_"+functionInfos[fi].varList[i]+", ";
    }
    stmt += "n, \n\t\t";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += "dim_"+functionInfos[fi].varList[i]+", ";
    }
    stmt += "\n\t\t";
    for (int i = 0; i < functionInfos[fi].constVarList.size(); ++i)
    {
        stmt += "const_"+functionInfos[fi].constVarList[i]+", ";
    }
    for (int i = 0; i < functionInfos[fi].arrayNameList.size(); ++i)
    {
        stmt += "const_"+functionInfos[fi].arrayNameList[i]+", ";
    }
    stmt += "\n\t\t";
    stmt += "row, col);\n";
    // llvm::outs()<<stmt<<"\n";
    return stmt;
  }

private:
  repl_map_t & repls_;
};

#endif
