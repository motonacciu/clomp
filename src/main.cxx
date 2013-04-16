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


std::string license = 
"========================================================\n\
University of Illinois/NCSA Open Source License\n\
\n\
Copyright (c) 2012 University of Innsbruck\n\
All rights reserved.\n\
\n\
Developed by:   Simone Pellegrini \n\
                University of Innsbruck \n\
                http://www.dps.uibk.ac.at/en/index.html \n\
========================================================\n";

int main(int argc, char* argv[]) {

	std::cout << license << std::endl;

	Program p;
	TranslationUnit& tu = p.addTranslationUnit(argv[1]);

	int c=0;
	for(auto it = p.pragmas_begin(), end = p.pragmas_end(); it != end; ++it) {
//		std::cout << "Pragma" << std::endl;
//
		PragmaPtr pragma = (*it).first;
		if (std::shared_ptr<omp::OmpPragma> omp_pragma = 
				std::dynamic_pointer_cast<omp::OmpPragma>(pragma)) 
		{
			std::cout << "OmpPragma: " << *omp_pragma->toAnnotation() << std::endl;
			c++;
		}
	}
	
	std::cout << c << " OpenMP pragmas" << std::endl;

}
