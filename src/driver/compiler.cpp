//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#include "driver/compiler.h"
#include "utils/config.h"
#include "sema.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"

#include "llvm/LLVMContext.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"

#include "llvm/Config/config.h"

#include "clang/Lex/Preprocessor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclGroup.h"

#include "clang/Parse/Parser.h"

#include <iostream>

using namespace clomp;
using namespace clang;

ParserProxy* ParserProxy::currParser = NULL;

clang::Expr* ParserProxy::ParseExpression(clang::Preprocessor& PP) {
	PP.Lex(mParser->Tok);

	Parser::ExprResult ownedResult = mParser->ParseExpression();
	Expr* result = ownedResult.takeAs<Expr> ();
	return result;
}

void ParserProxy::EnterTokenStream(clang::Preprocessor& PP) {
	PP.EnterTokenStream(&(CurrentToken()), 1, true, false);
}

Token& ParserProxy::ConsumeToken() {
	mParser->ConsumeAnyToken();
	// Token token = PP.LookAhead(0);
	return CurrentToken();
}

clang::Scope* ParserProxy::CurrentScope() {
	return mParser->getCurScope();
}

Token& ParserProxy::CurrentToken() {
	return mParser->Tok;
}

namespace {

void setDiagnosticClient(clang::CompilerInstance& clang) {

	// NOTE: the TextDiagnosticPrinter within the set DiagnosticClient takes over ownership of the printer object!
	clang::DiagnosticOptions* options = new clang::DiagnosticOptions();

	// set diagnostic options for the error reporting
	options->ShowLocation = 1;
	options->ShowCarets = 1;
	options->ShowColors = 1; // REMOVE FOR BETTER ERROR REPORT IN ECLIPSE
	options->TabStop = 4;

	TextDiagnosticPrinter* diagClient = new TextDiagnosticPrinter(llvm::errs(), options);
	// cppcheck-suppress exceptNew
	
	// check why, it might be a double insert in list, or a isolated delete somewhere
	DiagnosticsEngine* diags = new DiagnosticsEngine(llvm::IntrusiveRefCntPtr<DiagnosticIDs>( new DiagnosticIDs() ), 
													options, diagClient, true);
	clang.setDiagnostics(diags);
}

} // end anonymous namespace

namespace clomp {

struct ClangCompiler::ClangCompilerImpl {
	CompilerInstance clang;
	llvm::IntrusiveRefCntPtr<TargetOptions> TO;

	ClangCompilerImpl(): TO(new TargetOptions) { }
};

ClangCompiler::ClangCompiler(const std::string& file_name) : pimpl(new ClangCompilerImpl) {

	setDiagnosticClient(pimpl->clang);

	pimpl->clang.createFileManager();
	pimpl->clang.createSourceManager( pimpl->clang.getFileManager() );

	// A compiler invocation object has to be created in order for the diagnostic object to work
	CompilerInvocation* CI = new CompilerInvocation; // CompilerInvocation will be deleted by CompilerInstance
	CompilerInvocation::CreateFromArgs(*CI, 0, 0, pimpl->clang.getDiagnostics());
	pimpl->clang.setInvocation(CI);
	
	//setup headers 
	pimpl->clang.getHeaderSearchOpts().UseBuiltinIncludes = 0;
	pimpl->clang.getHeaderSearchOpts().UseStandardSystemIncludes = 1;  // Includes system includes, usually  /usr/include
	pimpl->clang.getHeaderSearchOpts().UseStandardCXXIncludes = 0;

	// Add default header 
	pimpl->clang.getHeaderSearchOpts().AddPath (CLANG_SYSTEM_INCLUDE_FOLDER,
			 									clang::frontend::System, true, false, false);

	// fix the target architecture to be a 64 bit machine
	pimpl->TO->Triple = llvm::Triple("x86_64", "PC", "Linux").getTriple();
	// TO.Triple = llvm::sys::getHostTriple();
	pimpl->clang.setTarget( TargetInfo::CreateTargetInfo (pimpl->clang.getDiagnostics(), *(pimpl->TO)) );


	std::string extension(file_name.substr(file_name.rfind('.')+1, std::string::npos));
	bool enableCpp = extension == "C" || extension == "cpp" || extension == "cxx" || extension == "hpp" || extension == "hxx";

	LangOptions& LO = pimpl->clang.getLangOpts();
	LO.GNUMode = 1;
	LO.Bool = 1;
	LO.POSIXThreads = 1;

	// if(CommandLineOptions::STD == "c99") LO.C99 = 1; 		// set c99

	if(enableCpp) {
		LO.CPlusPlus = 1; 	// set C++ 98 support
		LO.CXXOperatorNames = 1;
		//if(CommandLineOptions::STD == "c++0x") {
		//	LO.CPlusPlus0x = 1; // set C++0x support
		//}
		LO.RTTI = 1;
		LO.Exceptions = 1;
		LO.CXXExceptions = 1;
	}

	// Do this AFTER setting preprocessor options
	pimpl->clang.createPreprocessor();
	pimpl->clang.createASTContext();

	getPreprocessor().getBuiltinInfo().InitializeBuiltins(
			getPreprocessor().getIdentifierTable(),
			getPreprocessor().getLangOpts()
	);

	std::cout << file_name << std::endl;

	const FileEntry *FileIn = pimpl->clang.getFileManager().getFile(file_name, true);
	assert(FileIn && "file not found");
	pimpl->clang.getSourceManager().createMainFileID(FileIn);
	pimpl->clang.getDiagnosticClient().BeginSourceFile(
											pimpl->clang.getLangOpts(),
											&pimpl->clang.getPreprocessor());
}

ASTContext& ClangCompiler::getASTContext() const { 
	return pimpl->clang.getASTContext(); 
}
Preprocessor& ClangCompiler::getPreprocessor() const { 
	return pimpl->clang.getPreprocessor(); 
}
DiagnosticsEngine& ClangCompiler::getDiagnostics() const { 
	return pimpl->clang.getDiagnostics(); 
}
SourceManager& ClangCompiler::getSourceManager() const { 
	return pimpl->clang.getSourceManager(); 
}
TargetInfo& ClangCompiler::getTargetInfo() const { 
	return pimpl->clang.getTarget(); 
}

ClangCompiler::~ClangCompiler() {
	pimpl->clang.getDiagnostics().getClient()->EndSourceFile();
	delete pimpl;
}

} // End clomp namespace
