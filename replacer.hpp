#ifndef REPLACER_HPP
#define REPLACER_HPP

#include "common.h"

class Replacer : public MatchFinder::MatchCallback {
  using repl_map_t = std::map<std::string, clang::tooling::Replacements>;
public :
  explicit Replacer(repl_map_t& repls) : repls_(repls) {}

  std::string const funcName_ = "f_decl";

  auto funcMatcher()
  {
    return
    functionDecl(isExpansionInMainFile(),
      hasName("loop_pre"),
      hasBody(compoundStmt().bind("compound"))
    ).bind("loop_pre");
    // clang-format on
  }  // matcher

  void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("loop_pre");
    if(!FD){
        llvm::outs()<<"!!!can not find loop_pre\n";
        return;
    }

    auto path = Context->getSourceManager().getFilename(FD->getLocation()).str();
    auto idx = path.find_last_of('/');
    auto filename = path.substr(idx+1,-1);
    // llvm::outs()<<"filename: "<<filename<<"\n";
    if(std::strcmp(objFile.c_str(), filename.c_str())!=0) return;
    // const VarDecl *VD = Result.Nodes.getNodeAs<VarDecl>("var");

    SourceManager &sm(Context->getSourceManager());
    uint32_t const length =
      sm.getFileOffset(end_of_the_end(FD->getEndLoc(),sm)) - sm.getFileOffset(FD->getBeginLoc());
    // llvm::outs()<<length<<"\n";
    // llvm::outs()<<end_of_the_end(FD->getEndLoc(),sm).printToString(sm)<<"\n";
  
    // replace prepare-function name 
    // llvm::outs()<< FD->getNameInfo().getName().getAsString() << "\n";
    string stmt;
    for(int i=0;i<functionInfos.size();i++)
    {
        stmt += "void "+functionInfos[i].funcName + generateFuncName(i);
        stmt += "\n{";
        stmt += generateGetField(i);
        stmt += generateInoutList(i);
        stmt += generateRest(i);
        stmt += "}\n\n";
    }
    Replacement replFuncName(Context->getSourceManager(), FD->getBeginLoc(), length, stmt);

    // // llvm::outs()<<"stmt: "<<stmt<<"\n";
    // Replacement replCompoundLeft(Context->getSourceManager(), CS->getRBracLoc(), 0, stmt);

    if(repls_[filename].add(replFuncName)) {
      llvm::outs() << "Failed to enter replacement to map for file "
                << filename << "\n";
    }
  };

  string generateFuncName(int fi) {
    string stmt = "_pre(Region& reg";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += ", Word "+functionInfos[fi].fieldNameList[i];
    }
    for (int i = 0; i < functionInfos[fi].constVarList.size(); ++i)
    {
        stmt += ", "+functionInfos[fi].constTypeList[i]+" "+functionInfos[fi].constVarList[i];
    }
    for (int i = 0; i < functionInfos[fi].arrayNameList.size(); ++i)
    {
        stmt += ", "+functionInfos[fi].arrayTypeList[i]+" "+functionInfos[fi].arrayNameList[i];
    }
    stmt += ")";
    return stmt;
  }

  string generateGetField(int fi) {
    string stmt = "\n";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += "\tField<scalar>& ";
        stmt += "pre_"+functionInfos[fi].varList[i];
        stmt += " = reg.getField<scalar>(";
        stmt += functionInfos[fi].fieldNameList[i];
        stmt += ");\n";
        // llvm::outs()<<stmt<<"\n";
    }
    return stmt;
  };

  string generateInoutList(int fi) {
    string stmt = "\n\tArray<label32> inoutList;\n";
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += "\tinoutList.push_back(";
        stmt += std::to_string(functionInfos[fi].inoutList[i]);
        stmt += ");\n";
        // llvm::outs()<<stmt<<"\n";
    }
    return stmt;
  };

  // string generateDeclVar() {
  //   string stmt = "\n";
  //   for (int i = 0; i < varList.size(); ++i)
  //   {
  //       stmt += "\tscalar *";
  //       stmt += "var_"+varList[i];
  //       stmt += " = ";
  //       stmt += "field_"+varList[i];
  //       stmt += ".getLocalData();\n";
  //   }
  //   return stmt;
  // };

  // string generateGetDim() {
  //   string stmt = "\n";
  //   for (int i = 0; i < varList.size(); ++i)
  //   {
  //       stmt += "\tlabel ";
  //       stmt += "dim_"+varList[i];
  //       stmt += " = ";
  //       stmt += "field_"+varList[i];
  //       stmt += ".getDim();\n";
  //   }
  //   return stmt;
  // };

  // string generateGetSize() {
  //   string stmt = "\n";
  //   for (int i = 0; i < varList.size(); ++i)
  //   {
  //       stmt += "\tlabel ";
  //       stmt += "size_"+varList[i];
  //       stmt += " = ";
  //       stmt += "field_"+varList[i];
  //       stmt += ".getSize();\n";
  //   }
  //   return stmt;
  // };

  // string generateAddArray() {
  //   string stmt = "\n\tDataSet dataSet_edge, dataSet_vertex;\n";
  //   for (int i = 0; i < varList.size(); ++i)
  //   {
  //       stmt += "\taddSingleArray(";
  //       if(setTypeList[i]=="face") stmt += "dataSet_edge, ";
  //       else if(setTypeList[i]=="cell") stmt += "dataSet_vertex, ";
  //       else llvm::outs()<<"error\n";
  //       stmt += "dim_"+varList[i]+", ";
  //       stmt += "size_"+varList[i]+", ";
  //       // copyin
  //       if(inoutList[i]==1) stmt += "COPYIN, ";
  //       else if(inoutList[i]==2) stmt += "COPYOUT, ";
  //       else if(inoutList[i]==3) stmt += "COPYINOUT, ";
  //       else llvm::outs()<<"error\n";
  //       stmt += "var_"+varList[i]+");\n";
  //   }
  //   return stmt;
  // };

  // string generateChooseTopo() {
  //   string stmt = "\n\tArrayArray& topo = chooseTopology(";
  //   for (int i = 0; i < varList.size(); ++i)
  //   {
  //       stmt += "'"+setTypeList[i]+"'";
  //       stmt += ", ";
  //   }
  //   stmt.replace(stmt.size()-2, 2, ");");
  //   stmt += "\n";
  //   return stmt;
  // };

  string generateRest(int fi) {
    string stmt = "\n\tprepareField(&inoutList[0], ";
    stmt += "\""+functionInfos[fi].funcName+"\", ";
    stmt += std::to_string(functionInfos[fi].varList.size());
    for (int i = 0; i < functionInfos[fi].varList.size(); ++i)
    {
        stmt += ", &pre_"+functionInfos[fi].varList[i];
    }
    stmt += ");\n";

    stmt += "\tprepareConst(";
    stmt += std::to_string((functionInfos[fi].constVarList.size()+functionInfos[fi].arrayNameList.size())*2);
    for (int i = 0; i < functionInfos[fi].constVarList.size(); ++i)
    {
        stmt += ", &"+functionInfos[fi].constVarList[i];
        stmt += ", sizeof("+functionInfos[fi].constVarList[i]+")";
    }
    for (int i = 0; i < functionInfos[fi].arrayNameList.size(); ++i)
    {
        stmt += ", "+functionInfos[fi].arrayNameList[i];
        stmt += ", sizeof(*"+functionInfos[fi].arrayNameList[i]+")";
    }
    stmt += ");\n";
    return stmt;
  };

private:
  repl_map_t & repls_;
};



#endif
