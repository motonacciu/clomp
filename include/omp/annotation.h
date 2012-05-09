//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#pragma once

#include <memory>
#include <ostream>
#include <vector>
#include <cassert>

#include "utils/string_utils.h"

#define DEFINE_TYPE(Type) \
	class Type; \
	typedef std::shared_ptr<Type> Type##Ptr;

namespace clang {
class Expr;
class DeclRefExpr;
}

namespace clomp { namespace omp {

DEFINE_TYPE(BaseAnnotation);
DEFINE_TYPE(Annotation);
DEFINE_TYPE(Reduction);
DEFINE_TYPE(Schedule);
DEFINE_TYPE(Collapse);
DEFINE_TYPE(Default);
DEFINE_TYPE(For);
DEFINE_TYPE(Single);
DEFINE_TYPE(Parallel);
DEFINE_TYPE(ParallelFor);
DEFINE_TYPE(Barrier);
DEFINE_TYPE(Critical);
DEFINE_TYPE(Master);
DEFINE_TYPE(Flush);

/**
 * This is the root class for OpenMP annotations, be aware that this is not an
 * IR Annotation (see OmpBaseAnnotation).
 */
struct Annotation {
	virtual std::ostream& dump(std::ostream& out) const { 
		return out; 
	}
};

typedef std::shared_ptr<Annotation> AnnotationPtr;

struct Barrier: public Annotation {
	std::ostream& dump(std::ostream& out) const { 
		return out << "barrier"; 
	}
};

/**
 * Holds a list of identifiers, because omp statements can refer to global (or
 * static) variables the vector will hold both VariablePtr or MemebrExpressions.
 */
typedef std::vector<const clang::DeclRefExpr*> VarList;
typedef std::shared_ptr<VarList> VarListPtr;

struct Reduction {

	// operator = + or - or * or & or | or ^ or && or ||
	enum Operator { PLUS, MINUS, STAR, AND, OR, XOR, LAND, LOR };

	Reduction(const Operator& op, const VarListPtr& vars): op(op), vars(vars) { }
	const Operator& getOperator() const { return op; }
	const VarList& getVars() const { assert(vars); return *vars; }

	std::ostream& dump(std::ostream& out) const {
		return out << "reduction(" << opToStr(op) 
				   << ": " << utils::join(*vars) << ")";
	}

	static std::string opToStr(Operator op) {
		switch(op) {
		case PLUS: 	return "+";
		case MINUS: return "-";
		case STAR: 	return "*";
		case AND: 	return "&";
		case OR:	return "|";
		case XOR:	return "^";
		case LAND:	return "&&";
		case LOR:	return "||";
		}
		assert(false && "Operator doesn't exist");
	}

private:
	const Operator op;
	VarListPtr vars;
};

/**
 * Represents the OpenMP Schedule clause that may appears in for and parallelfor.
 * schedule( static | dynamic | guided | auto | runtime, [expression] )
 */
struct Schedule {

	enum Kind { STATIC, DYNAMIC, GUIDED, AUTO, RUNTIME };

	Schedule(const Kind& kind, const clang::Expr* chunkExpr): 
		kind(kind), chunkExpr(chunkExpr) { }

	const Kind& getKind() const { return kind; }
	bool hasChunkSizeExpr() const { return static_cast<bool>(chunkExpr); }
	const clang::Expr* getChunkSizeExpr() const { 
		assert(hasChunkSizeExpr()); 
		return chunkExpr; 
	}

	std::ostream& dump(std::ostream& out) const {
		out << "schedule(" << kindToStr(kind);
		if(hasChunkSizeExpr())
			out << ", " << chunkExpr;
		return out << ")";
	}

	static std::string kindToStr(Kind op) {
		switch(op) {
		case STATIC: 	return "static";
		case DYNAMIC: 	return "dynamic";
		case GUIDED: 	return "guided";
		case AUTO: 		return "auto";
		case RUNTIME: 	return "runtime";
		}
		assert(false && "Scheduling kind doesn't exist");
	}
private:
	Kind kind;
	const clang::Expr* chunkExpr;
};

/**
 * Represents the OpenMP Default clause that may appears in for and parallelfor.
 * default( shared | none )
 */
struct Default {

	enum Kind { SHARED, NONE };

	Default(const Kind& mode): mode(mode) { }
	const Kind& getMode() const { return mode; }

	std::ostream& dump(std::ostream& out) const {
		return out << "default(" << modeToStr(mode) << ")";
	}

	static std::string modeToStr(Kind op) {
		switch(op) {
		case SHARED: 	return "shared";
		case NONE: 		return "none";
		}
		assert(false && "Mode doesn't exist");
	}

private:
	Kind mode;
};

/**
 * OpenMP 'master' clause
 */
struct Master: public Annotation {
	std::ostream& dump(std::ostream& out) const { 
		return out << "master"; 
	}
};

/**
 * Represent clauses which are
 */
class ForClause {

protected:
	VarListPtr			lastPrivateClause;
	SchedulePtr			scheduleClause;
	const clang::Expr*	collapseExpr;
	bool 				noWait;

public:
	ForClause(const VarListPtr& lastPrivateClause, 
			  const SchedulePtr& scheduleClause, 
			  const clang::Expr* collapseExpr, 
			  bool noWait) 
	: lastPrivateClause(lastPrivateClause), 
		scheduleClause(scheduleClause), 
		collapseExpr(collapseExpr), 
		noWait(noWait) { }

	bool hasLastPrivate() const { 
		return static_cast<bool>(lastPrivateClause); 
	}
	const VarList& getLastPrivate() const { 
		assert(hasLastPrivate()); 
		return *lastPrivateClause; 
	}

	bool hasSchedule() const { 
		return static_cast<bool>(scheduleClause); 
	}
	const Schedule& getSchedule() const { 
		assert(hasSchedule()); 
		return *scheduleClause; 
	}

	bool hasCollapse() const { 
		return static_cast<bool>(collapseExpr); 
	}
	const clang::Expr* getCollapse() const { 
		assert(hasCollapse()); 
		return collapseExpr; 
	}

	bool hasNoWait() const { return noWait; }

	std::ostream& dump(std::ostream& out) const;
};

class SharedParallelAndTaskClause {

protected:
	const clang::Expr*	ifClause;
	DefaultPtr			defaultClause;
	VarListPtr			sharedClause;

public:
	SharedParallelAndTaskClause(const clang::Expr* ifClause, 
							    const DefaultPtr& defaultClause, 
								const VarListPtr& sharedClause) :
		ifClause(ifClause), 
		defaultClause(defaultClause), 
		sharedClause(sharedClause) { }

	bool hasIf() const { return static_cast<bool>(ifClause); }
	const clang::Expr* getIf() const { 
		assert(hasIf()); 
		return ifClause; 
	}

	bool hasDefault() const { 
		return static_cast<bool>(defaultClause); 
	}
	const Default& getDefault() const { 
		assert(hasDefault()); 
		return *defaultClause; 
	}

	bool hasShared() const { 
		return static_cast<bool>(sharedClause); 
	}
	const VarList& getShared() const { 
		assert(hasShared()); 
		return *sharedClause; 
	}

	std::ostream& dump(std::ostream& out) const;
};

class ParallelClause: public SharedParallelAndTaskClause {

protected:
	const clang::Expr* 	numThreadClause;
	VarListPtr			copyinClause;

public:
	ParallelClause(const clang::Expr* ifClause,
				   const clang::Expr* numThreadClause,
				   const DefaultPtr& defaultClause,
				   const VarListPtr& sharedClause,
				   const VarListPtr& copyinClause) :
			SharedParallelAndTaskClause(ifClause, defaultClause, sharedClause),
			numThreadClause(numThreadClause), 
			copyinClause(copyinClause) { }

	bool hasNumThreads() const { 
		return static_cast<bool>(numThreadClause); 
	}
	const clang::Expr* getNumThreads() const { 
		assert(hasNumThreads()); 
		return numThreadClause; 
	}

	bool hasCopyin() const { 
		return static_cast<bool>(copyinClause); 
	}
	const VarList& getCopyin() const { 
		assert(hasCopyin()); 
		return *copyinClause; 
	}

	std::ostream& dump(std::ostream& out) const;
};

class CommonClause {

protected:
	VarListPtr	privateClause;
	VarListPtr	firstPrivateClause;

public:
	CommonClause(const VarListPtr& privateClause, 
				 const VarListPtr& firstPrivateClause) :
			privateClause(privateClause), 
			firstPrivateClause(firstPrivateClause) { }

	bool hasPrivate() const { 
		return static_cast<bool>(privateClause); 
	}
	const VarList& getPrivate() const { 
		assert(hasPrivate()); 
		return *privateClause; 
	}

	bool hasFirstPrivate() const { 
		return static_cast<bool>(firstPrivateClause); 
	}
	const VarList& getFirstPrivate() const { 
		assert(hasFirstPrivate()); return *firstPrivateClause; 
	}

	std::ostream& dump(std::ostream& out) const;
};


/**
 * Interface to enable common access to data sharing clauses
 */
struct DatasharingClause : public CommonClause {

	DatasharingClause(const VarListPtr& privateClause, 
					  const VarListPtr& firstPrivateClause) :
		CommonClause(privateClause, firstPrivateClause) { }

	virtual bool hasReduction() const = 0;
	virtual const Reduction& getReduction() const = 0;
};

/**
 * OpenMP 'parallel' clause
 */
class Parallel: public DatasharingClause, 
				public Annotation, 
				public ParallelClause 
{
	ReductionPtr reductionClause;

public:
	Parallel(const clang::Expr* ifClause,
			 const clang::Expr* numThreadClause,
			 const DefaultPtr& defaultClause,
			 const VarListPtr& privateClause,
			 const VarListPtr& firstPrivateClause,
			 const VarListPtr& sharedClause,
			 const VarListPtr& copyinClause,
			 const ReductionPtr& reductionClause) :
		DatasharingClause(privateClause, firstPrivateClause),
		ParallelClause(ifClause, numThreadClause, defaultClause, sharedClause, copyinClause),
		reductionClause(reductionClause) { }

	bool hasReduction() const { 
		return static_cast<bool>(reductionClause); 
	}
	const Reduction& getReduction() const { 
		assert(hasReduction()); 
		return *reductionClause; 
	}

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'for' clause
 */
class For: public DatasharingClause, 
		   public Annotation, 
		   public ForClause 
{
	ReductionPtr reductionClause;

public:
	For(const VarListPtr&   privateClause,
		const VarListPtr&   firstPrivateClause,
		const VarListPtr&   lastPrivateClause,
		const ReductionPtr& reductionClause,
		const SchedulePtr&  scheduleClause,
		const clang::Expr*  collapseExpr,
		bool noWait) :
			DatasharingClause(privateClause, firstPrivateClause),
			ForClause(lastPrivateClause, scheduleClause, collapseExpr, noWait), 
			reductionClause(reductionClause) { }

	bool hasReduction() const { 
		return static_cast<bool>(reductionClause); 
	}
	const Reduction& getReduction() const { 
		assert(hasReduction()); 
		return *reductionClause; 
	}

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'parallel for' clause
 */
class ParallelFor: public Annotation, 
				   public CommonClause, 
				   public ParallelClause, 
				   public ForClause 
{

protected:
	ReductionPtr reductionClause;

public:
	ParallelFor(const clang::Expr*  ifClause,
				const clang::Expr*  numThreadClause,
				const DefaultPtr&   defaultClause,
				const VarListPtr&   privateClause,
				const VarListPtr&   firstPrivateClause,
				const VarListPtr&   sharedClause,
				const VarListPtr&   copyinClause,
				const ReductionPtr& reductionClause,
				const VarListPtr&   lastPrivateClause,
				const SchedulePtr&  scheduleClause,
				const clang::Expr*  collapseExpr, 
				bool noWait) :
		CommonClause(privateClause, firstPrivateClause),
		ParallelClause(ifClause, numThreadClause, defaultClause, sharedClause, copyinClause),
		ForClause(lastPrivateClause, scheduleClause, collapseExpr, noWait), 
		reductionClause(reductionClause) { }

	bool hasReduction() const { 
		return static_cast<bool>(reductionClause); 
	}
	const Reduction& getReduction() const { 
		assert(hasReduction()); 
		return *reductionClause; 
	}

	ParallelPtr toParallel() const {
		return std::make_shared<Parallel>(ifClause, 
				numThreadClause, 
				defaultClause, 
				privateClause, 
				firstPrivateClause, 
				sharedClause, 
				copyinClause, 
				reductionClause);
	}

	ForPtr toFor() const {
		// do not duplicate stuff already handled in parallel
		return std::make_shared<For>(
				/*private*/VarListPtr(), 
				/*firstprivate*/VarListPtr(), 
				lastPrivateClause, 
				/*reduction*/ReductionPtr(), 
				scheduleClause, 
				collapseExpr, 
				noWait);
	}

	std::ostream& dump(std::ostream& out) const;
};

class SectionClause {
	VarListPtr		lastPrivateClause;
	ReductionPtr	reductionClause;
	bool 			noWait;

public:
	SectionClause(const VarListPtr&   lastPrivateClause,
				  const ReductionPtr& reductionClause,
			      bool noWait) : 
		lastPrivateClause(lastPrivateClause), 
		reductionClause(reductionClause), 
		noWait(noWait) { }

	bool hasLastPrivate() const { 
		return static_cast<bool>(lastPrivateClause); 
	}
	const VarList& getLastPrivate() const { 
		assert(hasLastPrivate()); 
		return *lastPrivateClause; 
	}

	bool hasReduction() const { 
		return static_cast<bool>(reductionClause); 
	}
	const Reduction& getReduction() const { 
		assert(hasReduction()); 
		return *reductionClause; 
	}

	bool hasNoWait() const { return noWait; }

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'sections' clause
 */
struct Sections: public Annotation, 
				 public CommonClause, 
				 public SectionClause 
{

	Sections(const VarListPtr&  privateClause,
			const VarListPtr&   firstPrivateClause,
			const VarListPtr&   lastPrivateClause,
			const ReductionPtr& reductionClause,
			bool noWait) :
		CommonClause(privateClause, firstPrivateClause),
		SectionClause(lastPrivateClause, reductionClause, noWait) { }

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'parallel sections' clause
 */
struct ParallelSections: public Annotation, 
						public CommonClause, 
						public ParallelClause, 
						public SectionClause 
{

	ParallelSections(const clang::Expr* ifClause,
					const clang::Expr* numThreadClause,
					const DefaultPtr& defaultClause,
					const VarListPtr& privateClause,
					const VarListPtr& firstPrivateClause,
					const VarListPtr& sharedClause,
					const VarListPtr& copyinClause,
					const ReductionPtr& reductionClause,
					const VarListPtr& lastPrivateClause,
					bool noWait) :
		CommonClause(privateClause, firstPrivateClause),
		ParallelClause(ifClause, numThreadClause, defaultClause, sharedClause, copyinClause),
		SectionClause(lastPrivateClause, reductionClause, noWait) { }

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'section' clause
 */
struct Section: public Annotation {

	std::ostream& dump(std::ostream& out) const { 
		return out << "section"; 
	}

};

/**
 * OpenMP 'single' clause
 */
class Single: public Annotation, 
			  public CommonClause 
{

	VarListPtr	copyPrivateClause;
	bool 		noWait;

public:
	Single(const VarListPtr& privateClause,
		   const VarListPtr& firstPrivateClause,
		   const VarListPtr& copyPrivateClause,
		   bool noWait) :
		CommonClause(privateClause, firstPrivateClause),
		copyPrivateClause(copyPrivateClause), 
		noWait(noWait) { }

	bool hasCopyPrivate() const { 
		return static_cast<bool>(copyPrivateClause); 
	}
	const VarList& getCopyPrivate() const { 
		assert(hasCopyPrivate()); 
		return *copyPrivateClause; 
	}

	bool hasNoWait() const { return noWait; }

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'task' clause
 */
class Task: public Annotation, 
			public CommonClause, 
			public SharedParallelAndTaskClause 
{
	bool 	untied;

public:
	Task(const clang::Expr* ifClause,
		bool untied,
		const DefaultPtr& defaultClause,
		const VarListPtr& privateClause,
		const VarListPtr& firstPrivateClause,
		const VarListPtr& sharedClause) :
			CommonClause(privateClause, firstPrivateClause),
			SharedParallelAndTaskClause(ifClause, defaultClause, sharedClause), 
			untied(untied) { }

	bool hasUntied() const { return untied; }

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'taskwait' clause
 */
struct TaskWait: public Annotation {
	std::ostream& dump(std::ostream& out) const { 
		return out << "task wait"; 
	}
};

/**
 * OpenMP 'atomic' clause
 */
struct Atomic: public Annotation {
	std::ostream& dump(std::ostream& out) const { 
		return out << "atomic"; 
	}
};

/**
 * OpenMP 'critical' clause
 */
class Critical: public Annotation {
	std::string name;

public:
	Critical(const std::string& name): name(name) { }

	bool hasName() const { 
		return !name.empty(); 
	}
	const std::string& getName() const { 
		assert(hasName()); 
		return name; 
	}

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'ordered' clause
 */
struct Ordered: public Annotation {
	std::ostream& dump(std::ostream& out) const { 
		return out << "ordered"; 
	}
};

/**
 * OpenMP 'flush' clause
 */
class Flush: public Annotation {
	VarListPtr varList;

public:
	Flush(const VarListPtr& varList): varList(varList) { }

	bool hasVarList() const { 
		return static_cast<bool>(varList); 
	}
	const VarList& getVarList() const { 
		assert(hasVarList()); 
		return *varList; 
	}

	std::ostream& dump(std::ostream& out) const;
};

/**
 * OpenMP 'threadprivate' clause
 */
struct ThreadPrivate: public Annotation {
	std::ostream& dump(std::ostream& out) const;
};

} // End omp namespace
} // End clomp namespace

namespace std {

ostream& operator<<(ostream& os, const clomp::omp::Annotation& ann);

} // end std namespace
