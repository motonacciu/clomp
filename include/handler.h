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
#pragma once

#include <memory>
#include <sstream>
#include <map>

#include "matcher.h"
#include "sema.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Pragma.h>
#include <clang/Parse/Parser.h>

// clang's forward declaration
namespace clang {
class Stmt;
class Decl;
class Expr;
}

namespace clomp { 

// ------------------------------------ Pragma ---------------------------
/**
 * Defines a generic pragma which contains the location (start,end), and the
 * target node
 */
class Pragma {
	/**
	 * Attach the pragma to a statement. If the pragma is already bound to a
	 * statement or location, a call to this method will produce an error.
	 */
	void setStatement(clang::Stmt const* stmt);

	/**
	 * Attach the pragma to a declaration. If the pragma is already bound to a
	 * statement or location, a call to this method will produce an error.
	 */
	void setDecl(clang::Decl const* decl);

	friend class clomp::ClompSema;
public:

	/**
	 * Type representing the target node which could be wither a statement
	 * or a declaration
	 */
	typedef llvm::PointerUnion<clang::Stmt const*, clang::Decl const*> PragmaTarget;

	/**
	 * Creates an empty pragma starting from source location startLoc and ending
	 * ad endLoc.
	 */
	Pragma(const clang::SourceLocation& startLoc, 
		   const clang::SourceLocation& endLoc, 
		   const std::string& 			type) :
		mStartLoc(startLoc), mEndLoc(endLoc), mType(type) { }

	/**
	 * Creates a pragma starting from source location startLoc and ending ad endLoc
	 * by passing the content of the map which associates, for each key defined in
	 * the pragma_matcher, the relative parsed list of values.
	 *
	 */
	Pragma(const clang::SourceLocation& startLoc, 
		   const clang::SourceLocation& endLoc, 
		   const std::string& 			type, 
		   const MatchMap& 				mmap) : mStartLoc(startLoc), mEndLoc(endLoc), mType(type) { }

	const clang::SourceLocation& getStartLocation() const { return mStartLoc; }
	const clang::SourceLocation& getEndLocation() const { return mEndLoc; }
	/**
	 * Returns a string which identifies the pragma
	 */
	const std::string& getType() const { return mType; }

	clang::Stmt const* getStatement() const;
	clang::Decl const* getDecl() const;

	/**
	 * Returns true if the AST node associated to this pragma is a statement (clang::Stmt)
	 */
	bool isStatement() const { 
		return !mTargetNode.isNull() && mTargetNode.is<clang::Stmt const*> ();	
	}

	/**
	 * Returns true if the AST node associated to this pragma is a declaration (clang::Decl)
	 */
	bool isDecl() const { 
		return !mTargetNode.isNull() && mTargetNode.is<clang::Decl const*> (); 
	}

	/**
	 * Writes the content of the pragma to standard output
	 */
	virtual void dump(std::ostream& out, const clang::SourceManager& sm) const;

	/**
	 * Returns a string representation of the pragma
	 */
	std::string toStr(const clang::SourceManager& sm) const;

	virtual ~Pragma() {	}

private:
	clang::SourceLocation mStartLoc, mEndLoc;
	std::string mType;
	PragmaTarget mTargetNode;
};

typedef std::shared_ptr<Pragma> PragmaPtr;
typedef std::vector<PragmaPtr> 	PragmaList;

// ------------------------------------ PragmaStmtMap ---------------------------
/**
 * Maps statements and declarations to a Pragma.
 */
class PragmaStmtMap {
public:
	typedef std::multimap<const clang::Stmt*, const PragmaPtr> StmtMap;
	typedef std::multimap<const clang::Decl*, const PragmaPtr> DeclMap;

	template <class IterT>
	PragmaStmtMap(const IterT& begin, const IterT& end) {
		std::for_each(begin, end, [ this ](const typename IterT::value_type& pragma){
			if(pragma.first->isStatement())
				this->stmtMap.insert( std::make_pair(pragma.first->getStatement(), pragma.first) );
			else
				this->declMap.insert( std::make_pair(pragma.first->getDecl(), pragma.first) );
		});
	}

	const StmtMap& getStatementMap() const { return stmtMap; }
	const DeclMap& getDeclarationMap() const { return declMap; }

private:
	StmtMap stmtMap;
	DeclMap declMap;
};

// -------------------------------- BasicPragmaHandler<T> ---------------------------
/**
 * Defines a generic pragma handler which uses the pragma_matcher. Pragmas which are syntactically
 * correct are then instantiated and associated with the following node (i.e. a Stmt or
 * Declaration). If an error occurs, the error message is printed out showing the location and the
 * list of tokens the parser was expecting at that location.
 */
template<class T>
class BasicPragmaHandler: public clang::PragmaHandler {
	node* pragma_matcher;
	std::string base_name;

public:
	BasicPragmaHandler(clang::IdentifierInfo* 	name, 
					   const node& 				pragma_matcher, 
					   const std::string& 		base_name = std::string()) 
		: PragmaHandler(name->getName().str()), 
		  pragma_matcher(pragma_matcher.copy()), base_name(base_name) { }

	void HandlePragma(clang::Preprocessor& 			PP, 
					  clang::PragmaIntroducerKind 	kind, 
					  clang::Token& 				FirstToken) 
	{
		// '#' symbol is 1 position before
		clang::SourceLocation&& startLoc = 
			ParserProxy::get().CurrentToken().getLocation().getLocWithOffset(-1);

		MatchMap mmap;
		ParserStack errStack;

		if ( pragma_matcher->MatchPragma(PP, mmap, errStack) ) {
			// the pragma type is formed by concatenation of the base_name and identifier, for
			// example the type for the pragma:
			//		#pragma omp barrier
			// will be "omp::barrier", the string is passed to the pragma constructur which store
			// the value
			std::ostringstream pragma_name;
			if(!base_name.empty())
				pragma_name << base_name << "::";
			if(!getName().empty())
				pragma_name << getName().str();

			clang::SourceLocation endLoc = ParserProxy::get().CurrentToken().getLocation();
			// the pragma has been successfully parsed, now we have to instantiate the correct type
			// which is associated to this pragma (T) and pass the matcher map in order for the
			// pragma to initialize his internal representation. The framework will then take care
			// of associating the pragma to the following node (i.e. a statement or a declaration).
			static_cast<ClompSema&>(ParserProxy::get().getParser()->getActions()).
				ActOnPragma<T>( pragma_name.str(), mmap, startLoc, endLoc );
			return;
		}
		// In case of error, we report it to the console using the clang Diagnostics.
		errorReport(PP, startLoc, errStack);
		PP.DiscardUntilEndOfDirective();
	}

	~BasicPragmaHandler() {	delete pragma_matcher; }
};

// ------------------------------------ PragmaHandlerFactory ---------------------------
struct PragmaHandlerFactory {

	template<class T>
	static clang::PragmaHandler* CreatePragmaHandler(
			clang::IdentifierInfo* name, node 
			const& re, 
			const std::string& base_name = std::string())
	{
		return new BasicPragmaHandler<T> (name, re, base_name);
	}
};

} // end clomp namespace


