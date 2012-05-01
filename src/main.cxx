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
