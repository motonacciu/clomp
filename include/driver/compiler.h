//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
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
