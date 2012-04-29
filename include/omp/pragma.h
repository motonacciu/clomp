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
