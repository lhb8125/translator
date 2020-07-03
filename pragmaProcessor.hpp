#ifndef PRAGMAPROCESSOR_HPP
#define PRAGMAPROCESSOR_HPP


#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PPCallbacks.h>
// using namespace clang::module;

class Find_Includes : public PPCallbacks
{
public:
  explicit inline Find_Includes(const clang::CompilerInstance &compiler)
    : compiler_(compiler) {}


  void PragmaMessage (SourceLocation Loc,
    StringRef Namespace,
    PragmaMessageKind Kind,
    StringRef Str)
  {
    SourceManager &sm(compiler_.getSourceManager());
    // llvm::outs()<<sm.getFileOffset(Loc)<<": "<<Str.str()<<"\n";
    if(Str.str()=="getField") {}
    else if(Str.str()=="getTopology") {}
    else if(Str.str()=="compute") {}
    else
    {
      arrayToken(Str.str());
      arrayLocList.push_back(sm.getFileOffset(Loc));
    }
    arrayTypeList.resize(arrayNameList.size());
  }

private:
  const clang::CompilerInstance &compiler_;
  void arrayToken(std::string str)
  {
    std::string var;
    int index=0;
    int i=0;
    while(i<str.length() && str[i]!='[')
    {
        var.push_back(str[i++]);
    }
    // llvm::outs()<<var<<"\n";
    i++;
    while(i<str.length() && str[i]!=']')
    {
      index = index*10+str[i++]-48;
    }
    // llvm::outs()<<index<<"\n";
    arrayNameList.push_back(var);
    arraySizeList.push_back(index);
  }
};

class PragmaProcessorAction : public clang::PreprocessOnlyAction
{
protected:
    // virtual void ExecuteAction()
    // {
    //     std::unique_ptr<Find_Includes> find_includes_callback(
    //         new Find_Includes(getCompilerInstance()));
    //     getCompilerInstance().getPreprocessor().addPPCallbacks(
    //         std::move(find_includes_callback)
    //     );

    //     clang::PreprocessOnlyAction::ExecuteAction();
    // }
  bool BeginSourceFileAction(CompilerInstance &ci)
  {
    std::unique_ptr<Find_Includes> find_includes_callback(
        new Find_Includes(getCompilerInstance()));

    Preprocessor &pp = ci.getPreprocessor();
    pp.addPPCallbacks(std::move(find_includes_callback));

    return true;
  }
  void EndSourceFileAction()
  {
    CompilerInstance &ci = getCompilerInstance();
    Preprocessor &pp = ci.getPreprocessor();
    Find_Includes *find_includes_callback = (Find_Includes*)(pp.getPPCallbacks());

  }
};


#endif