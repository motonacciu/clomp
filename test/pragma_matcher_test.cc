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

#include <gtest/gtest.h>

#include "driver/program.h"
#include "driver/compiler.h"
#include "utils/source_locations.h"
#include "clang_config.h"

#include "handler.h"
#include "omp/pragma.h"

#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"

using namespace clomp;

#define CHECK_LOCATION(loc, srcMgr, line, col) \
	EXPECT_EQ(utils::Line(loc, srcMgr), (size_t)line); \
	EXPECT_EQ(utils::Column(loc, srcMgr), (size_t)col);

TEST(PragmaMatcherTest, HandleOmpParallel) {

	Program prog;
	TranslationUnit& tu = prog.addTranslationUnit( std::string(SRC_DIR) + "/inputs/omp_parallel.c" );

	const PragmaList& pl = tu.getPragmaList();
	const ClangCompiler& comp = tu.getCompiler();

	EXPECT_FALSE(pl.empty());
	EXPECT_EQ(pl.size(), (size_t) 4);

	// first pragma is at location [(4:2) - (4:22)]
	PragmaPtr p = pl[0];
	{
		// check pragma start location
		CHECK_LOCATION(p->getStartLocation(), comp.getSourceManager(), 4, 2);
		// check pragma end location
		CHECK_LOCATION(p->getEndLocation(), comp.getSourceManager(), 4, 22);

		EXPECT_EQ(p->getType(), "omp::parallel");

		// pragma associated to a statement
		EXPECT_TRUE(p->isStatement());
		const clang::Stmt* stmt = p->getStatement();

		// check stmt start location
		CHECK_LOCATION(stmt->getLocStart(), comp.getSourceManager(), 5, 2);
		// check stmt end location
		CHECK_LOCATION(stmt->getLocEnd(), comp.getSourceManager(), 9, 2);

		// check the omp parallel is empty
		omp::OmpPragma* omp = static_cast<omp::OmpPragma*>(p.get());
		EXPECT_TRUE(omp->getMap().empty());
	}

	// CHECK SECOND PRAGMA
	p = pl[1];
	{
		// check pragma start location
		CHECK_LOCATION(p->getStartLocation(), comp.getSourceManager(), 12, 2);
		// check pragma end location
		CHECK_LOCATION(p->getEndLocation(), comp.getSourceManager(), 12, 60);

		EXPECT_EQ(p->getType(), "omp::parallel");

		// pragma associated to a statement
		EXPECT_TRUE(p->isStatement());
		const clang::Stmt* stmt = p->getStatement();

		// check stmt start location
		CHECK_LOCATION(stmt->getLocStart(), comp.getSourceManager(), 13, 2);
		// check stmt end location
		CHECK_LOCATION(stmt->getLocEnd(), comp.getSourceManager(), 13, 4);

		// check the omp parallel is empty
		omp::OmpPragma* omp = static_cast<omp::OmpPragma*>(p.get());
		EXPECT_FALSE(omp->getMap().empty());

		auto fit = omp->getMap().find("private");
		EXPECT_TRUE( fit != omp->getMap().end() );
		const ValueList& values = fit->second;
		// only 1 variable in the private construct
		EXPECT_EQ(values.size(), (size_t) 2);

		// check first variable name
		{
			clang::DeclRefExpr* varRef =  llvm::dyn_cast<clang::DeclRefExpr>(values[0]->get<clang::Stmt*>());
			ASSERT_TRUE(varRef);
			// ASSERT_EQ(varRef->getDecl()->getNameAsString(), "a");
		}

		// check second variable name
		{
			clang::DeclRefExpr* varRef = llvm::dyn_cast<clang::DeclRefExpr>(values[1]->get<clang::Stmt*>());
			ASSERT_TRUE(varRef);
			ASSERT_EQ(varRef->getDecl()->getNameAsString(), "b");
		}

		// check default(shared)
		auto dit = omp->getMap().find("default");
		EXPECT_TRUE(dit != omp->getMap().end());
		EXPECT_FALSE(dit->second.empty());
		EXPECT_EQ(*dit->second[0]->get<std::string*>(), "shared");
	}

	p = pl[2];
	{
		// check pragma start location
		CHECK_LOCATION(p->getStartLocation(), comp.getSourceManager(), 15, 2);
		// check pragma end location
		CHECK_LOCATION(p->getEndLocation(), comp.getSourceManager(), 15, 20);

		EXPECT_EQ(p->getType(), "omp::master");

		// pragma associated to a statement
		EXPECT_TRUE(p->isStatement());
		const clang::Stmt* stmt = p->getStatement();

		// check stmt start location
		CHECK_LOCATION(stmt->getLocStart(), comp.getSourceManager(), 16, 2);
		// check stmt end location
		CHECK_LOCATION(stmt->getLocEnd(), comp.getSourceManager(), 16, 24);

		// check the omp parallel is empty
		omp::OmpPragma* omp = static_cast<omp::OmpPragma*>(p.get());
		EXPECT_TRUE(omp->getMap().empty());
	}

	p = pl[3];
	{
		// check pragma start location
		CHECK_LOCATION(p->getStartLocation(), comp.getSourceManager(), 19, 2);
		// check pragma end location
		CHECK_LOCATION(p->getEndLocation(), comp.getSourceManager(), 19, 20);

		EXPECT_EQ(p->getType(), "omp::single");

		// pragma associated to a statement
		EXPECT_TRUE(p->isStatement());
		const clang::Stmt* stmt = p->getStatement();

		// check stmt start location
		CHECK_LOCATION(stmt->getLocStart(), comp.getSourceManager(), 20, 2);
		// check stmt end location
		CHECK_LOCATION(stmt->getLocEnd(), comp.getSourceManager(), 20, 23);

		// check the omp parallel is empty
		omp::OmpPragma* omp = static_cast<omp::OmpPragma*>(p.get());
		EXPECT_TRUE(omp->getMap().empty());
	}

}

TEST(PragmaMatcherTest, HandleOmpFor) {

	try {
	Program prog;
	prog.addTranslationUnit( std::string(SRC_DIR) + "/inputs/omp_for.c" );

	const PragmaList& pl = (*prog.getTranslationUnits().begin())->getPragmaList();
	const ClangCompiler& comp = (*prog.getTranslationUnits().begin())->getCompiler();

	EXPECT_FALSE(pl.empty());
	EXPECT_EQ(pl.size(), (size_t) 4);

	// first pragma is at location [(6:2) - (6:37)]
	PragmaPtr p = pl[0];
	{
		// check pragma start location
		CHECK_LOCATION(p->getStartLocation(), comp.getSourceManager(), 6, 2);
		// check pragma end location
		CHECK_LOCATION(p->getEndLocation(), comp.getSourceManager(), 6, 37);

		EXPECT_EQ(p->getType(), "omp::parallel");

		// pragma associated to a statement
		EXPECT_TRUE(p->isStatement());
		const clang::Stmt* stmt = p->getStatement();

		// check stmt start location
		CHECK_LOCATION(stmt->getLocStart(), comp.getSourceManager(), 7, 2);
		// check stmt end location
		CHECK_LOCATION(stmt->getLocEnd(), comp.getSourceManager(), 9, 2);

		// check the omp parallel is empty
		omp::OmpPragma* omp = static_cast<omp::OmpPragma*>(p.get());
		EXPECT_FALSE(omp->getMap().empty());

		// look for 'for' keyword in the map
		EXPECT_TRUE(omp->getMap().find("for") != omp->getMap().end());

		auto fit = omp->getMap().find("private");
		EXPECT_TRUE(fit != omp->getMap().end());
		const ValueList& values = fit->second;
		// only 1 variable in the private construct
		EXPECT_EQ(values.size(), (size_t) 1);

		// check first variable name
		{
			clang::DeclRefExpr* varRef = llvm::dyn_cast<clang::DeclRefExpr>(values[0]->get<clang::Stmt*>());
			ASSERT_TRUE(varRef);
			ASSERT_EQ(varRef->getDecl()->getNameAsString(), "a");
		}
	}

	// pragma is at location [(11:2) - (11:22)]
	p = pl[1];
	{
		// check pragma start location
		CHECK_LOCATION(p->getStartLocation(), comp.getSourceManager(), 11, 2);
		// check pragma end location
		CHECK_LOCATION(p->getEndLocation(), comp.getSourceManager(), 11, 22);

		EXPECT_EQ(p->getType(), "omp::parallel");

		// pragma associated to a statement
		EXPECT_TRUE(p->isStatement());
		const clang::Stmt* stmt = p->getStatement();

		// check stmt start location
		CHECK_LOCATION(stmt->getLocStart(), comp.getSourceManager(), 12, 2);
		// check stmt end location
		CHECK_LOCATION(stmt->getLocEnd(), comp.getSourceManager(), 18, 2);

		// check empty map
		omp::OmpPragma* omp = static_cast<omp::OmpPragma*>(p.get());
		EXPECT_TRUE(omp->getMap().empty());
	}

	// pragma is at location [(13:3) - (14:14)]
	p = pl[2];
	{
		// check pragma start location
		CHECK_LOCATION(p->getStartLocation(), comp.getSourceManager(), 13, 3);
		// check pragma end location
		CHECK_LOCATION(p->getEndLocation(), comp.getSourceManager(), 14, 14);

		EXPECT_EQ(p->getType(), "omp::for");

		// pragma associated to a statement
		EXPECT_TRUE(p->isStatement());
		const clang::Stmt* stmt = p->getStatement();

		// check stmt start location
		CHECK_LOCATION(stmt->getLocStart(), comp.getSourceManager(), 15, 3);
		// check stmt end location
		CHECK_LOCATION(stmt->getLocEnd(), comp.getSourceManager(), 17, 3);

		// check the omp parallel is empty
		omp::OmpPragma* omp = static_cast<omp::OmpPragma*>(p.get());

		auto fit = omp->getMap().find("firstprivate");
		EXPECT_TRUE(fit != omp->getMap().end());
		const ValueList& values = fit->second;
		// only 1 variable in the private construct
		EXPECT_EQ(values.size(), (size_t) 1);

		// check first variable name
		{
			clang::DeclRefExpr* varRef =  llvm::dyn_cast<clang::DeclRefExpr>(values[0]->get<clang::Stmt*>());
			ASSERT_TRUE(varRef);
			ASSERT_EQ(varRef->getDecl()->getNameAsString(), "a");
		}

		// look for 'nowait' keyword in the map
		EXPECT_TRUE(omp->getMap().find("nowait") != omp->getMap().end());
	}

	// pragma is at location [(16:5) - (16:24)]
	p = pl[3];
	{
		// check pragma start location
		CHECK_LOCATION(p->getStartLocation(), comp.getSourceManager(), 16, 5);
		// check pragma end location
		CHECK_LOCATION(p->getEndLocation(), comp.getSourceManager(), 16, 24);

		EXPECT_EQ(p->getType(), "omp::barrier");

		// pragma associated to a statement
		EXPECT_TRUE(p->isStatement());
		const clang::Stmt* stmt = p->getStatement();

		EXPECT_TRUE( llvm::dyn_cast<clang::NullStmt>(stmt) != NULL );
		EXPECT_TRUE( stmt->getLocStart().isInvalid() );
	}

	} catch (std::exception e) {
		assert(false);
		std::cerr << e.what() << std::endl;
	}

}
