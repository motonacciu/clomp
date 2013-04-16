//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#include "driver/program.h"

#include "handler.h"
#include "omp/pragma.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclGroup.h"
#include "clang/Analysis/CFG.h"

#include "clang/Parse/Parser.h"
#include "clang/Parse/ParseAST.h"

#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaConsumer.h"
#include "clang/Sema/ExternalSemaSource.h"

using namespace clomp;
using namespace clang;

namespace {

/*
 * Instantiate the clang parser and sema to build the clang AST. Pragmas are
 * stored during the parsing
 */
void parseClangAST(ClangCompiler&		comp, 
				   clang::ASTConsumer*	Consumer, 
				   bool 				CompleteTranslationUnit, 
				   PragmaList& 			PL) 
{
	ClompSema S(PL, comp.getPreprocessor(), 
		 		comp.getASTContext(), *Consumer, 
				CompleteTranslationUnit
		 	   );

	Parser P(comp.getPreprocessor(), S, false);
	comp.getPreprocessor().EnterMainSourceFile();

	P.Initialize();
	ParserProxy::init(&P);
	Consumer->Initialize(comp.getASTContext());
	if (SemaConsumer *SC = dyn_cast<SemaConsumer>(Consumer)) {
		SC->InitializeSema(S);
	}

	if (ExternalASTSource *External = comp.getASTContext().getExternalSource()) {
		if(ExternalSemaSource *ExternalSema = dyn_cast<ExternalSemaSource>(External))
			ExternalSema->InitializeSema(S);
		External->StartTranslationUnit(Consumer);
	}

	Parser::DeclGroupPtrTy ADecl;
	while(!P.ParseTopLevelDecl(ADecl))
		if(ADecl) Consumer->HandleTopLevelDecl(ADecl.getAsVal<DeclGroupRef>());

	Consumer->HandleTranslationUnit(comp.getASTContext());
	ParserProxy::discard();

	S.dump();
}

} // end anonymous namespace

namespace clomp {

TranslationUnit::TranslationUnit(const std::string& file_name): 
	mFileName(file_name), mClang(file_name)  
{
	// register 'omp' pragmas
	omp::registerPragmaHandlers( mClang.getPreprocessor() );

	clang::ASTConsumer emptyCons;
	parseClangAST(mClang, &emptyCons, true, mPragmaList);

	if( mClang.getDiagnostics().hasErrorOccurred() ) {
		// errors are always fatal!
		throw ClangParsingError(file_name);
	}

}

struct Program::ProgramImpl {
	TranslationUnitSet tranUnits;

	ProgramImpl() { }
};

Program::Program(): pimpl( new ProgramImpl() ) { }
Program::~Program() { delete pimpl; }

TranslationUnit& Program::addTranslationUnit(const std::string& file_name) {
	auto tu = std::make_shared<TranslationUnit>(file_name);
	/* the shared_ptr will take care of cleaning the memory */;
	pimpl->tranUnits.insert( tu );
	return *tu;
}

const Program::TranslationUnitSet& Program::getTranslationUnits() const { 
	return pimpl->tranUnits; 
}

Program::PragmaIterator Program::pragmas_begin() const {
	auto filtering = [](const Pragma&) -> bool { return true; };
	return Program::PragmaIterator(pimpl->tranUnits, filtering);
}

Program::PragmaIterator Program::pragmas_end() const {
	return Program::PragmaIterator(pimpl->tranUnits.end());
}

bool Program::PragmaIterator::operator!=(const PragmaIterator& iter) const {
	return (tuIt != iter.tuIt); // FIXME also compare the pragmaIt value
}

void Program::PragmaIterator::inc(bool init) {
	while(tuIt != tuEnd) {
		if(init)	pragmaIt = (*tuIt)->getPragmaList().begin();
		// advance to the next pragma if there are still pragmas in the
		// current translation unit
		if(!init && pragmaIt != (*tuIt)->getPragmaList().end()) { ++pragmaIt; }

		if(pragmaIt != (*tuIt)->getPragmaList().end() && filteringFunc(**pragmaIt)) {
			return;
		}
		// advance to the next translation unit
		++tuIt;
		if(tuIt != tuEnd)
			pragmaIt = (*tuIt)->getPragmaList().begin();
	}
}

std::pair<PragmaPtr, TranslationUnitPtr> Program::PragmaIterator::operator*() const {
	assert(tuIt != tuEnd && pragmaIt != (*tuIt)->getPragmaList().end());
	return std::pair<PragmaPtr, TranslationUnitPtr>(*pragmaIt, *tuIt);
}

} // end clomp namespace

