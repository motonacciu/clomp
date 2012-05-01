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
#include "driver/program.h"

#include "handler.h"
#include "omp/pragma.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclGroup.h"
#include "clang/Analysis/CFG.h"

#include "clang/Index/TranslationUnit.h"
#include "clang/Index/DeclReferenceMap.h"
#include "clang/Index/SelectorMap.h"

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

	Parser P(comp.getPreprocessor(), S);
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

class TranslationUnitImpl: 
	public clomp::TranslationUnit,
	public idx::TranslationUnit 
{
	std::shared_ptr<idx::DeclReferenceMap>  mDeclRefMap;
	std::shared_ptr<idx::SelectorMap>		mSelMap;

public:
	TranslationUnitImpl(const std::string& file_name):
		clomp::TranslationUnit(file_name) 
	{
		// register 'omp' pragmas
		omp::registerPragmaHandlers( mClang.getPreprocessor() );

		clang::ASTConsumer emptyCons;
		parseClangAST(mClang, &emptyCons, true, mPragmaList);

		if( mClang.getDiagnostics().hasErrorOccurred() ) {
			// errors are always fatal!
			throw ClangParsingError(file_name);
		}

		// the translation unit has been correctly parsed
		mDeclRefMap = 
			std::make_shared<idx::DeclReferenceMap>( mClang.getASTContext() );
		mSelMap = 
			std::make_shared<idx::SelectorMap>( mClang.getASTContext() );
	}

	// getters
	clang::Preprocessor& getPreprocessor() { 
		return getCompiler().getPreprocessor(); 
	}
	const clang::Preprocessor& getPreprocessor() const { 
		return getCompiler().getPreprocessor(); 
	}

	clang::ASTContext& getASTContext() { 
		return getCompiler().getASTContext(); 
	}
	const clang::ASTContext& getASTContext() const { 
		return getCompiler().getASTContext(); 
	}

	clang::DiagnosticsEngine& getDiagnostic() { 
		return getCompiler().getDiagnostics(); 
	}
	const clang::DiagnosticsEngine& getDiagnostic() const { 
		return getCompiler().getDiagnostics(); 
	}

	clang::idx::DeclReferenceMap& getDeclReferenceMap() { 
		assert(mDeclRefMap); 
		return *mDeclRefMap; 
	}
	clang::idx::SelectorMap& getSelectorMap() { 
		assert(mSelMap); 
		return *mSelMap; 
	}
};

} // end anonymous namespace

namespace clomp {

struct Program::ProgramImpl {
	TranslationUnitSet tranUnits;

	ProgramImpl() { }
};

Program::Program(): pimpl( new ProgramImpl() ) { }
Program::~Program() { delete pimpl; }

TranslationUnit& Program::addTranslationUnit(const std::string& file_name) {
	TranslationUnitImpl* tuImpl = new TranslationUnitImpl(file_name);
	/* the shared_ptr will take care of cleaning the memory */;
	pimpl->tranUnits.insert( TranslationUnitPtr(tuImpl) );
	return *tuImpl;
}

const Program::TranslationUnitSet& Program::getTranslationUnits() const { 
	return pimpl->tranUnits; 
}

const TranslationUnit& Program::getTranslationUnit(const idx::TranslationUnit* tu) {
	return *dynamic_cast<const TranslationUnit*>(
			reinterpret_cast<const TranslationUnitImpl*>(tu)
		);
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

