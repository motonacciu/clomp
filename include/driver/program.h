//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#pragma once

#include "compiler.h"

#include <set>
#include <memory>
#include <algorithm>

namespace clang {
namespace idx {
class TranslationUnit;
} // end idx namespace
} // end clang namespace


namespace clomp { 

class Pragma;
typedef std::shared_ptr<Pragma> PragmaPtr;
typedef std::vector<PragmaPtr> PragmaList;

// ------------------------------------ TranslationUnit ---------------------------
/**
 * A translation unit contains informations about the compiler (needed to keep
 * alive object instantiated by clang), and the pragmas encountred during the
 * processing of the translation unit.
 */
class TranslationUnit {

	// Make this class noncopyable
	TranslationUnit(const TranslationUnit&);

protected:
	std::string 			mFileName;
	ClangCompiler			mClang;
	PragmaList 				mPragmaList;

public:
	TranslationUnit() { }
	TranslationUnit(const std::string& fileName): 
		mFileName(fileName), mClang(fileName) { }
	/**
	 * Returns a list of pragmas defined in the translation unit
	 */
	const PragmaList& getPragmaList() const { return mPragmaList; }
	
	const ClangCompiler& getCompiler() const {  return mClang; }
	
	const std::string& getFileName() const { 	return mFileName; }
};

typedef std::shared_ptr<TranslationUnit> TranslationUnitPtr;

// ------------------------------------ Program ---------------------------
/**
 * A program is made of a set of compilation units, we need to keep this object
 * so we can create a complete call graph and thus determine which part of the
 * input program should be handled by insieme and which should be kept as the
 * original.
 */
class Program {

	// Implements the pimpl pattern so we don't need to introduce an explicit
	// dependency to Clang headers
	class ProgramImpl;
	typedef ProgramImpl* ProgramImplPtr;
	ProgramImplPtr pimpl;

	/* Makes this class non copyable */
	Program(const Program& other); 

public:
	typedef std::set<TranslationUnitPtr> TranslationUnitSet;

	Program();
	~Program();

	/**
	 * Add a single file to the program
	 */
	TranslationUnit& addTranslationUnit(const std::string& fileName);

	/**
	 * Returns a list of parsed translation units
	 */
	const TranslationUnitSet& getTranslationUnits() const;

	static const TranslationUnit& getTranslationUnit(const clang::idx::TranslationUnit* tu);

	class PragmaIterator: public 
				std::iterator<
						std::input_iterator_tag, 
						std::pair<clomp::PragmaPtr, TranslationUnitPtr>
				> 
	{
	public:
		typedef std::function<bool (const Pragma&)> FilteringFunc;

	private:
		TranslationUnitSet::const_iterator tuIt, tuEnd;
		clomp::PragmaList::const_iterator pragmaIt;
		FilteringFunc filteringFunc;

		// creates end iter
		PragmaIterator(const TranslationUnitSet::const_iterator& tend) : tuIt(tend), tuEnd(tend) { }
		PragmaIterator(const TranslationUnitSet& tu, const FilteringFunc& filteringFunc):
			tuIt(tu.begin()), tuEnd(tu.end()), filteringFunc(filteringFunc) { inc(true); }

		void inc(bool init);

		friend class Program;

	public:
		bool operator!=(const PragmaIterator& iter) const;
		bool operator==(const PragmaIterator& iter) const { return !(*this != iter); }
		std::pair<PragmaPtr, TranslationUnitPtr> operator*() const;
		PragmaIterator& operator++() { inc(false); return *this; }
	};
	/**
	 * Returns the list of registered pragmas across the translation units
	 */
	PragmaIterator pragmas_begin() const;
	PragmaIterator pragmas_end() const;
};

} // end clomp namespace
