#include "common.h"
#include <unistd.h>
#include "replacer.hpp"
#include "analyzer.hpp"
#include "kernelReplacer.hpp"
#include "skeletonReplacer.hpp"
#include "pragmaProcessor.hpp"

//StatementMatcher InoutMatcher =
//	declRefExpr(isExpansionInMainFile()).bind("inout");




int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  Tool.run(newFrontendActionFactory<PragmaProcessorAction>().get());

  MatchFinder funcFinder;
  FuncPrinter funcPrinter;
  funcFinder.addMatcher(FuncMatcher, &funcPrinter);
  Tool.run(newFrontendActionFactory(&funcFinder).get());

  MatchFinder Finder;
  LoopPrinter loopPrinter;
  VarPrinter varPrinter;
  InoutPrinter inoutPrinter;
  TopoPrinter topoPrinter;
  ConstPrinter constPrinter;
  ArrayPrinter arrayPrinter;
  Finder.addMatcher(loopPrinter.forStmtMatcher(), &loopPrinter);
  Finder.addMatcher(getFieldMatcher, &varPrinter);
  Finder.addMatcher(InoutMatcher, &inoutPrinter);
  Finder.addMatcher(getTopoMatcher, &topoPrinter);
  Finder.addMatcher(constMatcher, &constPrinter);
  Finder.addMatcher(arrayMatcher, &arrayPrinter);
  Tool.run(newFrontendActionFactory(&Finder).get());


  std::vector<std::string> fileList;
  fileList.push_back(objFile);
  fileList.push_back(slaveFile);
  initPreFile();
  initSlaveFile();
  RefactoringTool refactTool(OptionsParser.getCompilations(),
                             fileList);
  MatchFinder replaceFinder;

  Replacer replacer(refactTool.getReplacements());
  replaceFinder.addMatcher(replacer.funcMatcher(), &replacer);

  KernelReplacer kernelReplacer(refactTool.getReplacements());
  replaceFinder.addMatcher(kernelReplacer.kernelMatcher(), &kernelReplacer);

  SkeletonReplacer skeletonReplacer(refactTool.getReplacements());
  replaceFinder.addMatcher(skeletonReplacer.kernelMatcher(), &skeletonReplacer);

  refactTool.runAndSave(newFrontendActionFactory(&replaceFinder).get());

  for (int i = 0; i < functionInfos.size(); ++i)
  {
    functionInfos[i].print();
  }

  // replace kernel
  RefactoringTool topoTool(OptionsParser.getCompilations(),
                             fileList);
  MatchFinder topoFinder;

  TopoReplacer topoReplacer(topoTool.getReplacements());
  SubscriReplacer subscriReplacer(topoTool.getReplacements());
  PointerReplacer pointerReplacer(topoTool.getReplacements());

  topoFinder.addMatcher(topoReplacer.topoMatcher(), &topoReplacer);
  topoFinder.addMatcher(subscriReplacer.subscriMatcher(), &subscriReplacer);
  topoFinder.addMatcher(pointerReplacer.pointerMatcher(), &pointerReplacer);
  
  topoTool.runAndSave(newFrontendActionFactory(&topoFinder).get());

  // for(auto & p : refactTool.getReplacements()) {
  //   auto & fname = p.first;
  //   auto & repls = p.second;
  //   llvm::outs() << "Replacements collected for file \"" << fname
  //             << "\" (not applied):\n";
  //   for(auto & r : repls) { llvm::outs() << "\t" << r.toString() << "\n"; }
  // }

  // for(auto & p : topoTool.getReplacements()) {
  //   auto & fname = p.first;
  //   auto & repls = p.second;
  //   llvm::outs() << "Replacements collected for file \"" << fname
  //             << "\" (not applied):\n";
  //   for(auto & r : repls) { llvm::outs() << "\t" << r.toString() << "\n"; }
  // }
  return 0;
}

void initSlaveFile()
{
  fstream f(slaveFile, ios::out);
  if(f.bad())
  {
      llvm::outs() << "error\n";
      return;
  }
  f << "#include \"kernel_slave.h\"\n";
  f << "\nvoid loop_kernel()\n";
  f << "{\n";
  f << "}\n";
  f << "\nvoid loop_skeleton()\n";
  f << "{\n";
  f << "}\n";

  f.close();
}

void initPreFile()
{
  fstream f(objFile, ios::out);
  if(f.bad())
  {
      llvm::outs() << "error";
      return;
  }
  f << "#include \"region.hpp\"\n";
  f << "#include \"field.hpp\"\n";
  f << "#include \"utilities.hpp\"\n";
  f << "#include \"funPtr_host.hpp\"\n";
  f << "extern void prepareField(label32* inoutList, char* funcName, int nPara, ...);\n";
  f << "extern void prepareConst(int nPara, ...);\n";
  f << "void loop_pre()\n";
  f << "{\n";
  f << "}\n";
  
  f.close();
}

