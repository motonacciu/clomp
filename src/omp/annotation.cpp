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
	if(hasLastPrivate())
		out << "lastprivate(" 
			<< utils::join(var_to_names(*lastPrivateClause)) 
			<< "), ";

	if(hasSchedule())
		scheduleClause->dump(out) << ", ";
	if(hasCollapse())
		out << "collapse(" << collapseExpr << "), ";
	if(hasNoWait())
		out << "nowait, ";
	return out;
}

///----- SharedParallelAndTaskClause -----
std::ostream& SharedParallelAndTaskClause::dump(std::ostream& out) const {
	if(hasIf())
		out << "if(" << ifClause << "), ";
	if(hasDefault())
		defaultClause->dump(out) << ", ";
	if(hasShared())
		out << "shared(" << utils::join(var_to_names(*sharedClause)) << "), ";
	return out;
}

///----- ParallelClause -----
std::ostream& ParallelClause::dump(std::ostream& out) const {
	SharedParallelAndTaskClause::dump(out) << ",";
	if(hasNumThreads())
		out << "num_threads(" << numThreadClause << "), ";
	if(hasCopyin())
		out << "copyin(" << utils::join(var_to_names(*copyinClause)) << "), ";
	return out;
}

///----- CommonClause -----
std::ostream& CommonClause::dump(std::ostream& out) const {
	if(hasPrivate())
		out << "private(" 
			<< utils::join(var_to_names(*privateClause)) 
			<< "), ";
	if(hasFirstPrivate())
		out << "firstprivate(" 
			<< utils::join(var_to_names(*firstPrivateClause)) 
			<< "), ";
	return out;
}

///----- Parallel -----
std::ostream& Parallel::dump(std::ostream& out) const {
	out << "parallel(";
	CommonClause::dump(out);
	ParallelClause::dump(out);
	if(hasReduction())
		reductionClause->dump(out) << ", ";
	return out << ")";
}

///----- For -----
std::ostream& For::dump(std::ostream& out) const {
	out << "for(";
	CommonClause::dump(out);
	ForClause::dump(out);
	if(hasReduction()) {
		reductionClause->dump(out) << ", ";
	}
	return out << ")";
}

///----- ParallelFor -----
std::ostream& ParallelFor::dump(std::ostream& out) const {
	out << "parallel for(";
	CommonClause::dump(out);
	ParallelClause::dump(out);
	ForClause::dump(out);
	if(hasReduction()) {
		reductionClause->dump(out) << ", ";
	}
	return out << ")";
}

///----- SectionClause -----
std::ostream& SectionClause::dump(std::ostream& out) const {
	if(hasLastPrivate())
		out << "lastprivate(" << utils::join(var_to_names(*lastPrivateClause)) << "), ";
	if(hasReduction())
		reductionClause->dump(out) << ", ";
	if(hasNoWait())
		out << "nowait, ";
	return out;
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
	CommonClause::dump(out);
	ParallelClause::dump(out);
	SectionClause::dump(out);
	return out << ")";
}

///----- Single -----
std::ostream& Single::dump(std::ostream& out) const {
	out << "single(";
	CommonClause::dump(out);
	if(hasCopyPrivate())
		out << "copyprivate(" << utils::join(var_to_names(*copyPrivateClause)) << "), ";
	if(hasNoWait())
		out << "nowait";
	return out << ")";
}

///----- Task -----
std::ostream& Task::dump(std::ostream& out) const {
	out << "task(";
	CommonClause::dump(out);
	SharedParallelAndTaskClause::dump(out);
	if(hasUntied())
		out << "united";
	return out << ")";
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

} // end std namespace
