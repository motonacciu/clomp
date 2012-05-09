//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#include "matcher.h"
#include "utils/source_locations.h"
#include "utils/string_utils.h"

#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/Parser.h>
#include <clang/AST/Expr.h>
#include "clang/Sema/Sema.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Basic/Diagnostic.h"

#include <llvm/Support/raw_ostream.h>

using namespace clang;
using namespace clomp;

#include <sstream>

namespace {

void reportRecord( std::ostream& 					ss, 
				   ParserStack::LocErrorList const& errs, 
				   clang::SourceManager& 			srcMgr ) 
{
	std::vector<std::string> list;
	std::transform(errs.begin(), errs.end(), back_inserter(list),
			[](const ParserStack::Error& pe) { return pe.expected; }
		);

	ss << clomp::utils::join(list, " | ");
	ss << std::endl;
}

} // end anonymous namespace

namespace clomp { 

// ------------------------------------ ValueUnion ---------------------------
ValueUnion::~ValueUnion() {
	if ( ptrOwner && is<clang::Stmt*>() ) {
		assert(clangCtx && "Invalid ASTContext associated with this element.");
		clangCtx->Deallocate(get<Stmt*>());
	}
	if(ptrOwner && is<std::string*>())
		delete get<std::string*>();
}

std::string ValueUnion::toStr() const {
	std::string ret;
	llvm::raw_string_ostream rs(ret);
	if ( is<Stmt*>() ) {
		get<Stmt*>()->printPretty(rs, *clangCtx, 0, clang::PrintingPolicy(clangCtx->getLangOptions()));
	} else {
		rs << *get<std::string*>();
	}
	return rs.str();
}

std::ostream& ValueUnion::printTo(std::ostream& out) const {
	return out << toStr();
}

MatchMap::MatchMap(const MatchMap& other) {

	std::for_each(other.cbegin(), other.cend(), [ this ](const MatchMap::value_type& curr) {
		(*this)[curr.first] = ValueList();
		ValueList& currList = (*this)[curr.first];

		std::for_each(curr.second.cbegin(), curr.second.cend(), [ &currList ](const ValueList::value_type& elem) {
			currList.push_back( ValueUnionPtr( new ValueUnion(*elem, true) ) );
		});
	});
}

std::ostream& MatchMap::printTo(std::ostream& out) const {
	for_each(begin(), end(), [&] ( const MatchMap::value_type& cur ) { 
				out << "KEY: '" << cur.first << "' -> ";
	//			out << "[" << join(", ", cur.second, 
	//				[](std::ostream& out, const ValueUnionPtr& cur){ out << *cur; } ) << "]";
				out << std::endl;
			});
	return out;
}

// ------------------------------------ ParserStack ---------------------------

size_t ParserStack::openRecord() {
	mRecords.push_back( LocErrorList() );
	return mRecordId++;
}

void ParserStack::addExpected(size_t recordId, const Error& pe) { mRecords[recordId].push_back(pe); }

void ParserStack::discardRecord(size_t recordId) { mRecords[recordId] = LocErrorList(); }

size_t ParserStack::getFirstValidRecord() {
	for ( size_t i=0; i<mRecords.size(); ++i )
		if ( !mRecords[i].empty() ) return i;
	assert(false);
}

void ParserStack::discardPrevRecords(size_t recordId) {

	std::for_each(mRecords.begin(), mRecords.begin()+recordId, [](ParserStack::LocErrorList& cur) {
		cur = ParserStack::LocErrorList();
	} );
}

const ParserStack::LocErrorList& ParserStack::getRecord(size_t recordId) const { return mRecords[recordId]; }


/**
 * This function is used to report an error occurred during the pragma matching. Clang utilities are used
 * to report the carret location of the error.
 */
void errorReport(clang::Preprocessor& pp, clang::SourceLocation& pragmaLoc, ParserStack& errStack) {
	using namespace clomp::utils;

	std::string str;
	llvm::raw_string_ostream sstr(str);
	pragmaLoc.print(sstr, pp.getSourceManager());
	std::ostringstream ss;
	ss << sstr.str() << ": error: expected ";

	size_t err, ferr = errStack.getFirstValidRecord();
	err = ferr;
	SourceLocation errLoc = errStack.getRecord(err).front().loc;
	ss << "at location (" << Line(errLoc, pp.getSourceManager()) << ":" << Column(errLoc, pp.getSourceManager()) << ") ";
	bool first = true;
	do {
		if ( !errStack.getRecord(err).empty() && errStack.getRecord(err).front().loc == errLoc ) {
			!first && ss << "\t";

			reportRecord(ss, errStack.getRecord(err), pp.getSourceManager());
			first = false;
		}
		err++;
	} while(err < errStack.stackSize());

	pp.Diag(errLoc, pp.getDiagnostics().getCustomDiagID(DiagnosticsEngine::Error, ss.str()));
}

// ------------------------------------ node ---------------------------
concat node::operator>>(node const& n) const { return concat(*this, n); }
star node::operator*() const { return star(*this); }
choice node::operator|(node const& n) const { return choice(*this, n); }
option node::operator!() const { return option(*this); }

bool concat::match(clang::Preprocessor& PP, MatchMap& mmap, ParserStack& errStack, size_t recID) const {
	int id = errStack.openRecord();
	PP.EnableBacktrackAtThisPos();
	if (first->match(PP, mmap, errStack, id)) {
		errStack.discardPrevRecords(id);
		id = errStack.openRecord();
		if(second->match(PP, mmap, errStack, id)) {
			PP.CommitBacktrackedTokens();
			errStack.discardRecord(id);
			return true;
		}
	}
	PP.Backtrack();
	return false;
}

bool star::match(clang::Preprocessor& PP, MatchMap& mmap, ParserStack& errStack, size_t recID) const {
	while (getNode()->match(PP, mmap, errStack, recID))
		;
	return true;
}

bool choice::match(clang::Preprocessor& PP, MatchMap& mmap, ParserStack& errStack, size_t recID) const {
	int id = errStack.openRecord();
	PP.EnableBacktrackAtThisPos();
	if (first->match(PP, mmap, errStack, id)) {
		PP.CommitBacktrackedTokens();
		errStack.discardRecord(id);
		return true;
	}
	PP.Backtrack();
	PP.EnableBacktrackAtThisPos();
	if (second->match(PP, mmap, errStack, id)) {
		PP.CommitBacktrackedTokens();
		errStack.discardRecord(id);
		return true;
	}
	PP.Backtrack();
	return false;
}

bool option::match(clang::Preprocessor& PP, MatchMap& mmap, ParserStack& errStack, size_t recID) const {
	PP.EnableBacktrackAtThisPos();
	if (getNode()->match(PP, mmap, errStack, recID)) {
		PP.CommitBacktrackedTokens();
		return true;
	}
	PP.Backtrack();
	return true;
}

bool expr_p::match(clang::Preprocessor& PP, MatchMap& mmap, ParserStack& errStack, size_t recID) const {
	// ClangContext::get().getParser()->Tok.setKind(*firstTok);
	PP.EnableBacktrackAtThisPos();
	Expr* result = ParserProxy::get().ParseExpression(PP);

	if (result) {
		PP.CommitBacktrackedTokens();
		ParserProxy::get().EnterTokenStream(PP);
		PP.LookAhead(1); // THIS IS CRAZY BUT IT WORKS
		if (getMapName().size())
			mmap[getMapName()].push_back( ValueUnionPtr(
				new ValueUnion(result, &static_cast<clang::Sema&>(ParserProxy::get().getParser()->getActions()).Context)
			));
		return true;
	}
	PP.Backtrack();
	errStack.addExpected(recID, ParserStack::Error("expr", ParserProxy::get().CurrentToken().getLocation()));
	return false;
}

bool kwd::match(clang::Preprocessor& PP, MatchMap& mmap, ParserStack& errStack, size_t recID) const {
	clang::Token& token = ParserProxy::get().ConsumeToken();
	if (token.is(clang::tok::identifier) && ParserProxy::get().CurrentToken().getIdentifierInfo()->getName() == kw) {
		if(isAddToMap() && getMapName().empty())
			mmap[kw];
		else if(isAddToMap())
			mmap[getMapName()].push_back( ValueUnionPtr(new ValueUnion( kw )) );
		return true;
	}
	errStack.addExpected(recID, ParserStack::Error("\'" + kw + "\'", token.getLocation()));
	return false;
}
std::string TokenToStr(clang::tok::TokenKind token) {
	const char *name = clang::tok::getTokenSimpleSpelling(token);
	if(name)
		return std::string(name);
	else
		return std::string(clang::tok::getTokenName(token));
}

std::string TokenToStr(const clang::Token& token) {
	if (token.isLiteral()) {
		return std::string(token.getLiteralData(), token.getLiteralData() + token.getLength());
	} else {
		return TokenToStr(token.getKind());
	}
}

void AddToMap(clang::tok::TokenKind tok, Token const& token, bool resolve, std::string const& map_str, MatchMap& mmap) {
	if (!map_str.size()) { return; }

	Sema& A = ParserProxy::get().getParser()->getActions();

	// HACK: FIXME
	// this hacks make it possible that if we have a token and we just want its string value 
	// we do not invoke clang semantics action on it. 
	if (!resolve) {
		if (tok == clang::tok::identifier) {
			UnqualifiedId Name;
			Name.setIdentifier(token.getIdentifierInfo(), token.getLocation());
			mmap[map_str].push_back( 
				ValueUnionPtr(new ValueUnion(
					std::string(
						Name.Identifier->getNameStart(), 
						Name.Identifier->getLength()
					)
				))
			);
			return;
		}
		mmap[map_str].push_back( ValueUnionPtr(new ValueUnion(TokenToStr(token))) );
		return ;
	}

	// We want to use clang sema to actually get the Clang node which is found out of this
	// identifier 
	switch (tok) {
	case clang::tok::numeric_constant:
		mmap[map_str].push_back(ValueUnionPtr(
			new ValueUnion(A.ActOnNumericConstant(token).takeAs<IntegerLiteral>(), &static_cast<clang::Sema&>(A).Context))
		);
		break;
	case clang::tok::identifier: {
		UnqualifiedId Name;
		CXXScopeSpec ScopeSpec;
		Name.setIdentifier(token.getIdentifierInfo(), token.getLocation());
			
		mmap[map_str].push_back(
			ValueUnionPtr(
				new ValueUnion(
					A.ActOnIdExpression(ParserProxy::get().CurrentScope(), ScopeSpec, Name, false, false).takeAs<Stmt>(),
					&static_cast<clang::Sema&>(A).Context
				)
			));
		break;
	}
	default: {
		mmap[map_str].push_back( ValueUnionPtr(new ValueUnion(TokenToStr(token))) );
		break;
	}
	}
}

} // End clomp namespace
