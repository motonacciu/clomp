//*****************************************************************************
//   This file is part of Clomp.
//	 Copyright (C) 2012  Simone Pellegrini
//
//   Clomp is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//	 any later version.
//	 
//	 Clomp is distributed in the hope that it will be useful,
//	 but WITHOUT ANY WARRANTY; without even the implied warranty of
//	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	 GNU General Public License for more details.
//
//	 You should have received a copy of the GNU General Public License
//	 along with Clomp.  If not, see <http://www.gnu.org/licenses/>.
//*****************************************************************************
#include "driver/compiler.h"
#include "clang_config.h"
#include "sema.h"

// defines which are needed by LLVM
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/DiagnosticOptions.h"
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

void setDiagnosticClient(clang::CompilerInstance& clang, clang::DiagnosticOptions& diagOpts) {
	TextDiagnosticPrinter* diagClient = new TextDiagnosticPrinter(llvm::errs(), diagOpts);
	DiagnosticsEngine* diags = new DiagnosticsEngine(llvm::IntrusiveRefCntPtr<DiagnosticIDs>( new DiagnosticIDs() ), diagClient);
	// clang will take care of memory deallocation of diags
	clang.setDiagnostics(diags);
}

} // end anonymous namespace

namespace clomp {

struct ClangCompiler::ClangCompilerImpl {
	CompilerInstance clang;
	DiagnosticOptions diagOpts;
};

ClangCompiler::ClangCompiler() : pimpl(new ClangCompilerImpl){
	// pimpl->clang.setLLVMContext(new llvm::LLVMContext);

	setDiagnosticClient(pimpl->clang, pimpl->diagOpts);
	pimpl->clang.createFileManager();
	pimpl->clang.createSourceManager( pimpl->clang.getFileManager() );

	// A compiler invocation object has to be created in order for the diagnostic object to work
	CompilerInvocation* CI = new CompilerInvocation; // CompilerInvocation will be deleted by CompilerInstance
	CompilerInvocation::CreateFromArgs(*CI, 0, 0, pimpl->clang.getDiagnostics());
	pimpl->clang.setInvocation(CI);

	TargetOptions TO;
	// fix the target architecture to be a 64 bit machine:
	// 		in this way we don't have differences between the size of integer/float types across architecture
	TO.Triple = llvm::Triple("x86_64", "PC", "Linux").getTriple();
	pimpl->clang.setTarget( TargetInfo::CreateTargetInfo (pimpl->clang.getDiagnostics(), TO) );

	pimpl->clang.createPreprocessor();
	pimpl->clang.createASTContext();
}

ClangCompiler::ClangCompiler(const std::string& file_name) : pimpl(new ClangCompilerImpl) {
	// pimpl->clang.setLLVMContext(new llvm::LLVMContext);

	// set diagnostic options for the error reporting
	pimpl->diagOpts.ShowLocation = 1;
	pimpl->diagOpts.ShowCarets = 1;
	pimpl->diagOpts.ShowColors = 1; // REMOVE FOR BETTER ERROR REPORT IN ECLIPSE
	pimpl->diagOpts.TabStop = 4;

	setDiagnosticClient(pimpl->clang, pimpl->diagOpts);

	pimpl->clang.createFileManager();
	pimpl->clang.createSourceManager( pimpl->clang.getFileManager() );
	pimpl->clang.InitializeSourceManager(file_name);

	// A compiler invocation object has to be created in order for the diagnostic object to work
	CompilerInvocation* CI = new CompilerInvocation; // CompilerInvocation will be deleted by CompilerInstance
	CompilerInvocation::CreateFromArgs(*CI, 0, 0, pimpl->clang.getDiagnostics());
	pimpl->clang.setInvocation(CI);

	// Add default header
	pimpl->clang.getHeaderSearchOpts().AddPath( CLANG_SYSTEM_INCLUDE_FOLDER, clang::frontend::System, true, false, false);

	// add headers
	//std::for_each(CommandLineOptions::IncludePaths.begin(), CommandLineOptions::IncludePaths.end(),
	//	[ this ](std::string& curr) {
	//		this->pimpl->clang.getHeaderSearchOpts().AddPath( curr, clang::frontend::System, true, false, false);
	//	}
	//);

	TargetOptions TO;
	// fix the target architecture to be a 64 bit machine
	TO.Triple = llvm::Triple("x86_64", "PC", "Linux").getTriple();
	// TO.Triple = llvm::sys::getHostTriple();
	pimpl->clang.setTarget( TargetInfo::CreateTargetInfo (pimpl->clang.getDiagnostics(), TO) );

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

	// Enable OpenCL
	// LO.OpenCL = 1;
	LO.AltiVec = 1;
	LO.LaxVectorConversions = 1;

	// set -D macros
	//std::for_each(CommandLineOptions::Defs.begin(), CommandLineOptions::Defs.end(), [ this ](std::string& curr) {
	//	this->pimpl->clang.getPreprocessorOpts().addMacroDef(curr);
	//});

	// Do this AFTER setting preprocessor options
	pimpl->clang.createPreprocessor();
	pimpl->clang.createASTContext();

	pimpl->clang.getDiagnostics().getClient()->BeginSourceFile( LO, &pimpl->clang.getPreprocessor() );
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
