//*****************************************************************************
//   This file is part of Clomp.
//
//   Clomp is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//	 (at your option) any later version.
//	 
//	 Clomp is distributed in the hope that it will be useful,
//	 but WITHOUT ANY WARRANTY; without even the implied warranty of
//	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	 GNU General Public License for more details.
//
//	 You should have received a copy of the GNU General Public License
//	 along with Clomp.  If not, see <http://www.gnu.org/licenses/>.
//*****************************************************************************
#pragma once

// defines which are needed by LLVM
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "clang/Sema/Sema.h"
#include "clang/Sema/Ownership.h"

using clang::SourceLocation;

namespace clomp { 

// forward declarations for pragma
class Pragma;
typedef std::shared_ptr<Pragma> PragmaPtr;
typedef std::vector<PragmaPtr> 	PragmaList;

class MatchMap;

/**
 * This purpose of this class is to overload the behavior of clang parser in a
 * way every time an AST node is created, pending pragmas are correctly
 * associated to it.
 */
class ClompSema: public clang::Sema {
	class ClompSemaImpl;
	ClompSemaImpl* pimpl;

	bool isInsideFunctionDef;

	void matchStmt(clang::Stmt* 				S, 
				   const clang::SourceRange& 	bounds, 
				   const clang::SourceManager& 	sm, 
				   PragmaList& 					matched);

	ClompSema(const Sema& other);

public:
	ClompSema (PragmaList&   		pragma_list,
		 	  clang::Preprocessor& 			pp, 
		 	  clang::ASTContext& 			ctx, 
		  	  clang::ASTConsumer& 			consumer, 
		  	  bool 							CompleteTranslationUnit = true,
		  	  clang::CodeCompleteConsumer* 	CompletionConsumer = 0) ;
	
	~ClompSema();

	void addPragma(PragmaPtr P);

	clang::StmtResult ActOnCompoundStmt(clang::SourceLocation 	L, 
										clang::SourceLocation 	R, 
										clang::MultiStmtArg 	Elts, 
										bool			 		isStmtExpr );
								   
	clang::StmtResult ActOnIfStmt(  clang::SourceLocation 		IfLoc, 
									clang::Sema::FullExprArg 	CondVal, 
									clang::Decl* 				CondVar, 
									clang::Stmt* 				ThenVal,
									clang::SourceLocation 		ElseLoc, 
									clang::Stmt* 				ElseVal );	

	clang::StmtResult ActOnForStmt( clang::SourceLocation 		ForLoc, 
									clang::SourceLocation		LParenLoc, 
									clang::Stmt* 				First,
									clang::Sema::FullExprArg 	Second, 
									clang::Decl* 				SecondVar, 
									clang::Sema::FullExprArg 	Third, 
									clang::SourceLocation 		RParenLoc, 
									clang::Stmt* 				Body );
								  
	clang::Decl* ActOnStartOfFunctionDef(clang::Scope*		FnBodyScope, 
										 clang::Declarator&	D );
	
	clang::Decl* ActOnStartOfFunctionDef(clang::Scope *FnBodyScope, clang::Decl* D);

	clang::Decl* ActOnFinishFunctionBody(clang::Decl* Decl, clang::Stmt* Body);
	
	clang::Decl* ActOnDeclarator(clang::Scope *S, clang::Declarator &D);

//	clang::StmtResult ActOnDeclStmt(clang::Sema::DeclGroupPtrTy Decl, SourceLocation StartLoc, SourceLocation EndLoc);

	void ActOnTagFinishDefinition(clang::Scope* S, clang::Decl* TagDecl, clang::SourceLocation RBraceLoc);

	/**
	 * Register the parsed pragma.
	 */
	template <class T>
	void ActOnPragma(const std::string& 		name, 
					 const MatchMap& 			mmap, 
					 clang::SourceLocation 		startLoc, 
					 clang::SourceLocation 		endLoc) 
	{
		addPragma( std::make_shared<T>(startLoc, endLoc, name, mmap) );
	}
	
	/**
	 * Write into the logger information about the pragmas and their associatioation to AST nodes.
	 */
	void dump();
};

} // end clomp namespace

