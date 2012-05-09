//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#include "driver/program.h"
#include "handler.h"
#include "omp/pragma.h"
#include "omp/annotation.h"

#include <iostream>

using namespace clomp;

int main(int argc, char* argv[]) {

	Program p;
	TranslationUnit& tu = p.addTranslationUnit(argv[1]);

	for(auto it = p.pragmas_begin(), end = p.pragmas_end(); it != end; ++it) {
		std::cout << "Pragma" << std::endl;

		PragmaPtr pragma = (*it).first;
		if (std::shared_ptr<omp::OmpPragma> omp_pragma = 
				std::dynamic_pointer_cast<omp::OmpPragma>(pragma)) 
		{
			std::cout << "OmpPragma: " << *omp_pragma->toAnnotation() << std::endl;
		}
	}
	

}
