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
#pragma once 

#include <string>
#include <iostream>
#include <sstream>

namespace clomp { namespace utils {

template <class IterT>
std::string join(IterT begin, const IterT& end, const std::string& sep=",") {
	if (begin == end) { return ""; }
	std::ostringstream ss;
	ss << *begin++;
	for (;begin!=end;++begin) {
		ss << sep << *begin;
	}
	return ss.str();
}

template <class ContainerT>
std::string join(const ContainerT& cont, const std::string& sep=",") {
	return join(cont.begin(), cont.end(), sep);
}

} // end utils namespace
} // end clomp namespace 
