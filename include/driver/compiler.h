//*****************************************************************************
//   This file is part of Clomp.
//	 Copyright (C) 2012  Simone Pellegrini
//   
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
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>
#include <cassert>

#include <boost/utility.hpp>

// forward declarations
namespace clang {
class ASTContext;
class ASTConsumer;
class Preprocessor;
class DiagnosticsEngine;
class SourceManager;
class Parser;
class Token;
class Scope;
class Expr;
class TargetInfo;

namespace idx {
class Program;
class Indexer;
}
} // end clang namespace

class TypeConversion_FileTest_Test;
class StmtConversion_FileTest_Test;

// ------------------------------------ ParserProxy ---------------------------
/**
 * This is a proxy class which enables the access to internal clang features, i.e. Parser.
 * The main scope of this class is to handle the parsing of pragma(s) of the input file
 */
class ParserProxy {
	static ParserProxy* currParser;
	clang::Parser* mParser;

	ParserProxy(clang::Parser* parser): mParser(parser) { }
public:

	/**
	 * Initialize the proxy with the parser used to parse the current translation unit,
	 * call this method with a NULL parser causes an assertion.
	 */
	static void init(clang::Parser* parser=NULL) {
		assert(parser && "ParserProxy cannot be initialized with a NULL parser");
		currParser = new ParserProxy(parser);
	}

	/**
	 * the discard method is called when the Parser is no longer valid.
	 */
	static void discard() {
		delete currParser;
		currParser = NULL;
	}

	/**
	 * Returns the current parser, if not initialized an assertion is thrown.
	 */
	static ParserProxy& get() {
		assert(currParser && "Parser proxy not initialized.");
		return *currParser;
	}

	/**
	 * Parse an expression using the clang parser starting from the current token
	 */
	clang::Expr* ParseExpression(clang::Preprocessor& PP);
	void EnterTokenStream(clang::Preprocessor& PP);
	/**
	 * Consumes the current token (by moving the input stream pointer) and returns a reference to it
	 */
	clang::Token& ConsumeToken();
	clang::Scope* CurrentScope();
	/**
	 * Returns the last consumed token without advancing in the input stream
	 */
	clang::Token& CurrentToken();
	clang::Parser* getParser() const { return mParser; }
};

namespace clomp {
/**
 * Used to report a parsing error occurred during the parsing of the input file
 */
struct ClangParsingError: public std::logic_error {
	ClangParsingError(const std::string& file_name): std::logic_error(file_name) { }
};


// ------------------------------------ ClangCompiler ---------------------------
/**
 * ClangCompiler is a wrapper class for the Clang compiler main interfaces. The main goal is to hide implementation
 * details to the client.
 */
class ClangCompiler: boost::noncopyable {
	struct ClangCompilerImpl;

	ClangCompilerImpl* pimpl;
public:
	/**
	 * Creates an empty compiler instance, usefull for test cases
	 */
	ClangCompiler();

	/**
	 * Creates a compiler instance from an input file
	 */
	ClangCompiler(const std::string& file_name);

	/**
	 * Returns clang's ASTContext
	 * @return
	 */
	clang::ASTContext& getASTContext() const;

	/**
	 * Returns clang's SourceManager
	 * @return
	 */
	clang::SourceManager& getSourceManager() const;

	/**
	 * Returns clang's Prepocessor
	 * @return
	 */
	clang::Preprocessor& getPreprocessor() const;

	/**
	 * Returns clang's Diagnostics
	 * @return
	 */
	clang::DiagnosticsEngine& getDiagnostics() const;

	/**
	 * Returns clang's TargetInfo
	 * @return
	 */
	clang::TargetInfo& getTargetInfo() const;

	~ClangCompiler();
};

} // End clomp namespace
