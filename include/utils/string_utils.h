//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
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

template <class T>
std::string toString(const T& obj) {
	std::ostringstream ss;
	ss << obj;
	return ss.str();
}

} // end utils namespace
} // end clomp namespace 
