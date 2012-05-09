//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#pragma once

#include "handler.h"

#include <set>
#include <memory>

namespace clang {
class VarDecl;
} // end clang namespace 

namespace clomp { namespace omp {

class Annotation;
typedef std::shared_ptr<Annotation> AnnotationPtr;

/**
 * Base class for OpenMP pragmas
 */
class OmpPragma: public Pragma {
	MatchMap mMap;
public:
	OmpPragma(const clang::SourceLocation&  startLoc, 
			  const clang::SourceLocation&  endLoc, 
			  const std::string& 			name, 
			  const MatchMap& 				mmap);

	const MatchMap& getMap() const { return mMap; }

	virtual AnnotationPtr toAnnotation() const = 0;

	virtual ~OmpPragma() { }
};

/**
 * Registers the handlers for OpenMP pragmas
 */
void registerPragmaHandlers(clang::Preprocessor& pp);

} // End omp namespace
} // End clomp namespace
