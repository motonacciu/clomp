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
#include "handler.h"
#include "utils/source_locations.h"

#include "clang/AST/Stmt.h"
#include <llvm/Support/raw_ostream.h>
#include <clang/AST/Expr.h>

using namespace clang;
using namespace clomp;

namespace clomp { 

void Pragma::setStatement(clang::Stmt const* stmt) {
	assert(mTargetNode.isNull() && "Pragma already associated with an AST node");
	mTargetNode = stmt;
}

void Pragma::setDecl(clang::Decl const* decl) {
	assert(mTargetNode.isNull() && "Pragma already associated with an AST node");
	mTargetNode = decl;
}

clang::Stmt const* Pragma::getStatement() const {
	assert(!mTargetNode.isNull() && isStatement());
	return mTargetNode.get<clang::Stmt const*> ();
}

clang::Decl const* Pragma::getDecl() const {
	assert(!mTargetNode.isNull() && isDecl());
	return mTargetNode.get<clang::Decl const*> ();
}

std::string Pragma::toStr(const clang::SourceManager& sm) const {
	std::ostringstream ss;
	ss << "(" << utils::location(getStartLocation(), sm) 
		<< ", " << utils::location(getEndLocation(), sm) << "),\n\t";
	
	ss << (isStatement() ? "Stmt -> " : "Decl -> ") << "(";

	if(isStatement() && getStatement()) {
		
		ss << utils::location(getStatement()->getLocStart(), sm) << ", " <<
			  utils::location(getStatement()->getLocEnd(), sm);
	}
	else if(isDecl() && getDecl())
		ss << utils::location(getDecl()->getLocStart(), sm) << ", " <<
			  utils::location(getDecl()->getLocEnd(), sm);
	ss << ")";
	return ss.str();
}

void Pragma::dump(std::ostream& out, const clang::SourceManager& sm) const {
	out << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" <<
		   "|~> Pragma: " << getType() << " -> " << std::flush << toStr(sm) << "\n";
}

} // End clomp namespace
