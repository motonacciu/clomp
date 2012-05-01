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

namespace clang {
class SourceLocation;
class SourceRange;
class SourceManager;
}

namespace clomp { namespace utils {

std::string FileName(clang::SourceLocation const& l, clang::SourceManager const& sm);

std::string FileId(clang::SourceLocation const& l, clang::SourceManager const& sm);

unsigned Line(clang::SourceLocation const& l, clang::SourceManager const& sm);

std::pair<unsigned, unsigned> Line(clang::SourceRange const& r, clang::SourceManager const& sm);

unsigned Column(clang::SourceLocation const& l, clang::SourceManager const& sm);

std::pair<unsigned, unsigned> Column(clang::SourceRange const& r, clang::SourceManager const& sm);

std::string location(clang::SourceLocation const& l, clang::SourceManager const& sm);

} // End utils namespace
} // End clomp namespace

