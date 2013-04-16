//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#include "omp/annotation.h"
#include "handler.h"

#include <memory>
#include <algorithm>

namespace {

std::vector<std::string> var_to_names(const clomp::omp::VarList& vars) {
	std::vector<std::string> ret(vars.size());

	std::transform(vars.begin(), vars.end(), ret.begin(), 
		[](const clang::DeclRefExpr* cur) -> std::string { 
			const clang::NamedDecl* decl = llvm::dyn_cast<clang::NamedDecl>(cur->getDecl());
			assert(decl && "Wrong declaration type");
			return std::string(decl->getNameAsString());
		} );

	return ret;
}

} // end anonymout namespace 

namespace clomp { namespace omp {

///----- ForClause -----
std::ostream& ForClause::dump(std::ostream& out) const {

	std::vector<std::string> clause_str;
	std::ostringstream ss;

	if(hasLastPrivate()) {
		ss << "lastprivate(" 
			<< utils::join(var_to_names(*lastPrivateClause)) 
			<< ")";
		clause_str.emplace_back( ss.str() );
	}

	if(hasSchedule()) 
		clause_str.emplace_back( utils::toString(*scheduleClause) );

	if(hasCollapse()) {
		ss.str("");
		ss << "collapse(" << collapseExpr << ")";
		clause_str.emplace_back( ss.str() );
	}

	if(hasNoWait()) 
		clause_str.emplace_back( "nowait" );

	return out << utils::join(clause_str);
}

///----- SharedParallelAndTaskClause -----
std::ostream& SharedParallelAndTaskClause::dump(std::ostream& out) const {
	std::vector<std::string> clause_str;
	std::ostringstream ss;

	if(hasIf()) {
		ss.str("");
		ss << "if(" << ifClause << ")";
		clause_str.emplace_back( ss.str() );
	}

	if(hasDefault()) 
		clause_str.emplace_back( utils::toString(*defaultClause) );

	if(hasShared()) {
		ss.str("");
		ss << "shared(" << utils::join(var_to_names(*sharedClause)) << ")";
		clause_str.emplace_back( ss.str() );
	}

	return out << utils::join(clause_str);
}

///----- ParallelClause -----
std::ostream& ParallelClause::dump(std::ostream& out) const {
	std::vector<std::string> clause_str;

	std::ostringstream ss;
	SharedParallelAndTaskClause::dump(ss);
	clause_str.emplace_back( ss.str() ); 

	if(hasNumThreads()) {
		ss.str("");
		ss << "num_threads(" << numThreadClause << ")";
		clause_str.emplace_back( ss.str() );
	} 
	if(hasCopyin()) {
		ss.str("");
		ss << "copyin(" << utils::join(var_to_names(*copyinClause)) << ")";
		clause_str.emplace_back( ss.str() );
	}
	return out << utils::join(clause_str);
}

///----- CommonClause -----
std::ostream& CommonClause::dump(std::ostream& out) const {

	std::ostringstream ss;
	std::vector<std::string> clause_str;

	if(hasPrivate()) {
		ss.str("");
		ss << "private(" 
		   << utils::join(var_to_names(*privateClause)) 
	  	   << ")";
		clause_str.emplace_back( ss.str() );
	}
	if(hasFirstPrivate()) {
		ss.str("");
		ss << "firstprivate(" 
		   << utils::join(var_to_names(*firstPrivateClause)) 
		   << ")";
		clause_str.emplace_back( ss.str() );
	}

	return out << utils::join( clause_str );
}

///----- Parallel -----
std::ostream& Parallel::dump(std::ostream& out) const {
	std::vector<std::string> clause_str;
	std::ostringstream ss;

	out << "parallel(";
	{
		CommonClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	{ 
		ss.str("");
		ParallelClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	if(hasReduction()) {
		ss.str("");
		reductionClause->dump(ss);
		if(!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	return out << utils::join(clause_str) << ")";
}

///----- For -----
std::ostream& For::dump(std::ostream& out) const {
	std::vector<std::string> clause_str;
	std::ostringstream ss;
	out << "for(";
	{ 
		CommonClause::dump(ss);
		if(!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	{
		ss.str("");
		ForClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	if(hasReduction()) {
		ss.str("");
		reductionClause->dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	return out << utils::join(clause_str) << ")";
}

///----- ParallelFor -----
std::ostream& ParallelFor::dump(std::ostream& out) const {
	std::vector<std::string> clause_str;
	std::ostringstream ss;

	out << "parallel for(";
	{
		CommonClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	{
		ss.str("");
		ParallelClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	{
		ss.str("");
		ForClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	if(hasReduction()) {
		ss.str("");
		reductionClause->dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	return out << utils::join(clause_str) << ")";
}

///----- SectionClause -----
std::ostream& SectionClause::dump(std::ostream& out) const {
	std::vector<std::string> clause_str;
	std::ostringstream ss;
	if(hasLastPrivate()) {
		ss.str("");
		ss << "lastprivate(" << utils::join(var_to_names(*lastPrivateClause)) << ")";
		clause_str.emplace_back( ss.str() );
	}
	if(hasReduction()) {
		ss.str("");
		reductionClause->dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	if(hasNoWait()) 
		clause_str.emplace_back( "nowait" );

	return out << utils::join( clause_str );
}

///----- Sections -----
std::ostream& Sections::dump(std::ostream& out) const {
	out << "sections(";
	CommonClause::dump(out);
	return SectionClause::dump(out) << ")";
}

///----- ParallelSections -----
std::ostream& ParallelSections::dump(std::ostream& out) const {
	out << "parallel sections(";

	std::vector<std::string> clause_str;
	std::ostringstream ss;
	{
		CommonClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	{
		ss.str("");
		ParallelClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	{
		ss.str("");
		SectionClause::dump(ss);
		if (!ss.str().empty())
			clause_str.emplace_back( ss.str() );
	}
	return out << utils::join(clause_str) << ")";
}

///----- Single -----
std::ostream& Single::dump(std::ostream& out) const {
	out << "single(";
	std::vector<std::string> clause_str;
	std::ostringstream ss;
	{
		CommonClause::dump(ss);
		clause_str.emplace_back( ss.str() );
	}
	if (hasCopyPrivate()) {
		ss.str("");
		ss << "copyprivate(" << utils::join(var_to_names(*copyPrivateClause)) << ")";
		clause_str.emplace_back( ss.str() );
	}
	if (hasNoWait()) 
		clause_str.emplace_back( "nowait" );

	return out << utils::join(clause_str) << ")";
}

///----- Task -----
std::ostream& Task::dump(std::ostream& out) const {
	out << "task(";

	std::vector<std::string> clause_str;
	std::ostringstream ss;

	{ 
		CommonClause::dump(ss);
		clause_str.emplace_back( ss.str() );
	}
	{
		ss.str("");
		SharedParallelAndTaskClause::dump(ss);
		clause_str.emplace_back( ss.str() );
	}
	if(hasUntied()) 
		clause_str.emplace_back( "united" );

	return out << utils::join(clause_str) << ")";
}

///----- Critical -----
std::ostream& Critical::dump(std::ostream& out) const {
	out << "critical";
	if(hasName())
		out << "(" << name << ")";
	return out;
}

///----- Flush -----
std::ostream& Flush::dump(std::ostream& out) const {
	out << "flush";
	if(hasVarList())
		out << "(" << utils::join(var_to_names(*varList)) << ")";
	return out;
}

///----- ThreadPrivate -----
std::ostream& ThreadPrivate::dump(std::ostream& out) const {
	return out << "threadprivate";
}

} // End omp namespace
} // End clomp namespace

namespace std {

ostream& operator<<(ostream& os, const clomp::omp::Annotation& ann) {
	ann.dump(os);
	return os;
}

ostream& operator<<(ostream& os, const clomp::omp::Schedule& ann) {
	ann.dump(os);
	return os;
}

ostream& operator<<(ostream& os, const clomp::omp::Default& ann) {
	ann.dump(os);
	return os;
}

} // end std namespace
