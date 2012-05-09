//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#include "utils/source_locations.h"

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <sstream>

#include <llvm/Support/raw_ostream.h>

using namespace std;
using namespace clang;

namespace clomp { namespace utils {

string FileName(SourceLocation const& l, SourceManager const& sm) {
	PresumedLoc pl = sm.getPresumedLoc(l);
	return string(pl.getFilename());
}

string FileId(SourceLocation const& l, SourceManager const& sm) {
	string fn = FileName(l, sm);
	for(size_t i=0; i<fn.length(); ++i)
		switch(fn[i]) {
			case '/':
			case '\\':
			case '>':
			case '.':
				fn[i] = '_';
		}
	return fn;
}

unsigned Line(SourceLocation const& l, SourceManager const& sm) {
	PresumedLoc pl = sm.getPresumedLoc(l);
	return pl.getLine();
}

std::pair<unsigned, unsigned> Line(clang::SourceRange const& r, SourceManager const& sm) {
	return std::make_pair(Line(r.getBegin(), sm), Line(r.getEnd(), sm));
}

unsigned Column(SourceLocation const& l, SourceManager const& sm) {
	PresumedLoc pl = sm.getPresumedLoc(l);
	return pl.getColumn();
}

std::pair<unsigned, unsigned> Column(clang::SourceRange const& r, SourceManager const& sm) {
	return std::make_pair(Column(r.getBegin(), sm), Column(r.getEnd(), sm));
}

std::string location(clang::SourceLocation const& l, clang::SourceManager const& sm) {
	std::string str;
	llvm::raw_string_ostream ss(str);
	l.print(ss,sm);
	return ss.str();
}

} // End util namespace
} // End clomp namespace
