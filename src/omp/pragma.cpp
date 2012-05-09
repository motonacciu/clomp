//=============================================================================
//               	Clomp: A Clang-based OpenMP Frontend
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//=============================================================================
#include "omp/pragma.h"
#include "omp/annotation.h"

#include "handler.h"
#include "matcher.h"

#include "utils/source_locations.h"

#include <clang/Lex/Pragma.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>

using namespace std;

namespace {

using namespace clomp;
using namespace clomp::omp;

#define OMP_PRAGMA(TYPE) 	\
struct OmpPragma ## TYPE: public OmpPragma { \
	OmpPragma ## TYPE(const clang::SourceLocation& 	startLoc, \
				      const clang::SourceLocation& 	endLoc,	\
					  const std::string& 			name, \
					  const MatchMap& 				mmap):	\
		OmpPragma(startLoc, endLoc, name, mmap) { }	\
	virtual omp::AnnotationPtr toAnnotation() const; 	\
}

// Defines basic OpenMP pragma types which will be created by the pragma_matcher class
OMP_PRAGMA(Parallel);
OMP_PRAGMA(For);
OMP_PRAGMA(Sections);
OMP_PRAGMA(Section);
OMP_PRAGMA(Single);
OMP_PRAGMA(Task);
OMP_PRAGMA(Master);
OMP_PRAGMA(Critical);
OMP_PRAGMA(Barrier);
OMP_PRAGMA(TaskWait);
OMP_PRAGMA(Atomic);
OMP_PRAGMA(Flush);
OMP_PRAGMA(Ordered);
OMP_PRAGMA(ThreadPrivate);

} // End anonymous namespace

namespace clomp { namespace omp {

void registerPragmaHandlers(clang::Preprocessor& pp) {
	using namespace clomp::tok;

	// if(scalar-expression)
	auto if_expr 		   	= kwd("if") >> l_paren >> tok::expr["if"] >> r_paren;

	// default(shared | none)
	auto def			   	= Tok<clang::tok::kw_default>() >> l_paren >>
							  ( kwd("shared") | kwd("none") )["default"] >> r_paren;

	// identifier *(, identifier)
	auto var_list   		= var >> *(~comma >> var);

	// private(list)
	auto private_clause    	=  kwd("private") >> l_paren >> var_list["private"] >> r_paren;

	// firstprivate(list)
	auto firstprivate_clause = kwd("firstprivate") >> l_paren >> var_list["firstprivate"] >> r_paren;

	// lastprivate(list)
	auto lastprivate_clause = kwd("lastprivate") >> l_paren >> var_list["lastprivate"] >> r_paren;

	// + or - or * or & or | or ^ or && or ||
	auto op 			  	= tok::plus | tok::minus | tok::star | tok::amp |
							  tok::pipe | tok::caret | tok::ampamp | tok::pipepipe;

	// reduction(operator: list)
	auto reduction_clause 	= kwd("reduction") >> l_paren >> op["reduction_op"] >> colon >>
							  var_list["reduction"] >> r_paren;

	auto parallel_clause =  ( 	// if(scalar-expression)
								if_expr
							| 	// num_threads(integer-expression)
								(kwd("num_threads") >> l_paren >> expr["num_threads"] >> r_paren)
							|	// default(shared | none)
								def
							|	// private(list)
								private_clause
							|	// firstprivate(list)
								firstprivate_clause
							|	// shared(list)
								(kwd("shared") >> l_paren >> var_list["shared"] >> r_paren)
							|	// copyin(list)
								(kwd("copyin") >> l_paren >> var_list["copyin"] >> r_paren)
							|	// reduction(operator: list)
								reduction_clause
							);

	auto kind 			=   Tok<clang::tok::kw_static>() | kwd("dynamic") | kwd("guided") | kwd("auto") | kwd("runtime");

	auto for_clause 	=	(	private_clause
							|	firstprivate_clause
							|	lastprivate_clause
							|	reduction_clause
								// schedule( (static | dynamic | guided | atuo | runtime) (, chunk_size) )
							|	(kwd("schedule") >> l_paren >> kind["schedule"] >>
									!( comma >> expr["chunk_size"] ) >> r_paren)
								// collapse( expr )
							|	(kwd("collapse") >> l_paren >> expr["collapse"] >> r_paren)
								// ordered
							|   kwd("ordered")
								// nowait
							|	kwd("nowait")
							);

	auto for_clause_list = !(for_clause >> *( !comma >> for_clause ));

	auto sections_clause =  ( 	// private(list)
								private_clause
							| 	// firstprivate(list)
								firstprivate_clause
							|	// lastprivate(list)
								lastprivate_clause
							|	// reduction(operator: list)
								reduction_clause
							| 	// nowait
								kwd("nowait")
							);

	auto sections_clause_list = !(sections_clause >> *( !comma >> sections_clause ));

	// [clause[ [, ]clause] ...] new-line
	auto parallel_for_clause_list = (parallel_clause | for_clause | sections_clause) >>
										*( !comma >> (parallel_clause | for_clause | sections_clause) );

	auto parallel_clause_list = !( 	(Tok<clang::tok::kw_for>("for") >> !parallel_for_clause_list)
								 |  (kwd("sections") >> !parallel_for_clause_list)
								 | 	(parallel_clause >> *(!comma >> parallel_clause))
								 );

	auto single_clause 	= 	(	// private(list)
								private_clause
							|	// firstprivate(list)
								firstprivate_clause
							|	// copyprivate(list)
							 	kwd("copyprivate") >> l_paren >> var_list["copyprivate"] >> r_paren
							|	// nowait
								kwd("nowait")
							);

	auto single_clause_list = !(single_clause >> *( !comma >> single_clause ));

	auto task_clause	 = 	(	// if(scalar-expression)
								if_expr
							|	// untied
								kwd("untied")
							|	// default(shared | none)
								def
							|	// private(list)
								private_clause
							| 	// firstprivate(list)
								firstprivate_clause
							|	// shared(list)
								kwd("shared") >> l_paren >> var_list["shared"] >> r_paren
							);

	auto task_clause_list = !(task_clause >> *( !comma >> task_clause ));

	// threadprivate(list)
	auto threadprivate_clause = l_paren >> var_list["thread_private"] >> r_paren;

	// define a PragmaNamespace for omp
	clang::PragmaNamespace* omp = new clang::PragmaNamespace("omp");
	pp.AddPragmaHandler(omp);

	// Add an handler for pragma omp parallel:
	// #pragma omp parallel [clause[ [, ]clause] ...] new-line
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaParallel>(
			pp.getIdentifierInfo("parallel"), parallel_clause_list >> tok::eod, "omp")
		);

	// omp for
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaFor>(
			pp.getIdentifierInfo("for"), for_clause_list >> tok::eod, "omp")
		);

	// #pragma omp sections [clause[[,] clause] ...] new-line
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaSections>(
			pp.getIdentifierInfo("sections"), sections_clause_list >> tok::eod, "omp")
		);

	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaSection>(
			pp.getIdentifierInfo("section"), tok::eod, "omp")
		);

	// omp single
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaSingle>(
			pp.getIdentifierInfo("single"), single_clause_list >> tok::eod, "omp")
		);

	// #pragma omp task [clause[[,] clause] ...] new-line
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaTask>(
			pp.getIdentifierInfo("task"), task_clause_list >> tok::eod, "omp")
		);

	// #pragma omp master new-line
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaMaster>(
			pp.getIdentifierInfo("master"), tok::eod, "omp")
		);

	// #pragma omp critical [(name)] new-line
	omp->AddPragma( PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaCritical>(
			pp.getIdentifierInfo("critical"), !(l_paren >> identifier["critical"] >> r_paren) >> tok::eod, "omp")
		);

	//#pragma omp barrier new-line
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaBarrier>(
			pp.getIdentifierInfo("barrier"), tok::eod, "omp")
		);

	// #pragma omp taskwait newline
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaTaskWait>(
			pp.getIdentifierInfo("taskwait"), tok::eod, "omp")
		);

	// #pragma omp atimic newline
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaAtomic>(
			pp.getIdentifierInfo("atomic"), tok::eod, "omp")
		);

	// #pragma omp flush [(list)] new-line
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaFlush>(
			pp.getIdentifierInfo("flush"), !(l_paren >> var_list["flush"] >> r_paren) >> tok::eod, "omp")
		);

	// #pragma omp ordered new-line
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaOrdered>(
			pp.getIdentifierInfo("ordered"), tok::eod, "omp")
		);

	// #pragma omp threadprivate(list) new-line
	omp->AddPragma(PragmaHandlerFactory::CreatePragmaHandler<OmpPragmaThreadPrivate>(
			pp.getIdentifierInfo("threadprivate"), threadprivate_clause >> tok::eod, "omp")
		);
}


OmpPragma::OmpPragma(const clang::SourceLocation& startLoc, 
					 const clang::SourceLocation& endLoc, 
					 const string& name,
					 const MatchMap& mmap) : 
	Pragma(startLoc, endLoc, name, mmap), mMap(mmap) 
{
	std::cout << "~ OmpPragma ~" << std::endl;
	for(MatchMap::const_iterator i = mmap.begin(), e = mmap.end(); i!=e; ++i) {
		std::vector<std::string> strs(i->second.size());
		std::transform(i->second.begin(), i->second.end(), strs.begin(), 
				[](const ValueUnionPtr& cur){ return cur->toStr(); }
			);
		std::cout << "KEYWORD: " << i->first << ":\n\t{" << utils::join(strs) << "}" << std::endl;
	}
	std::cout << "~~~~~~~~~~~~~" << std::endl;
}

} // End omp namespace
} // End clomp namespace

namespace {

using namespace clomp;

/**
 * Create an annotation with the list of identifiers, used for clauses: private,firstprivate,lastprivate
 */
VarListPtr handleIdentifierList(const MatchMap& mmap, const std::string& key) {

	auto fit = mmap.find(key);
	if(fit == mmap.end())
		return VarListPtr();

	const ValueList& vars = fit->second;
	VarList* varList = new VarList;
	for(ValueList::const_iterator it = vars.begin(), end = vars.end(); it != end; ++it) {
		clang::Stmt* varIdent = (*it)->get<clang::Stmt*>();
		assert(varIdent && "Clause not containing var exps");

		clang::DeclRefExpr* refVarIdent = llvm::dyn_cast<clang::DeclRefExpr>(varIdent);
		assert(refVarIdent && "Clause not containing a DeclRefExpr");
		varList->push_back( refVarIdent );
	}
	return VarListPtr( varList );
}

// reduction(operator: list)
// operator = + or - or * or & or | or ^ or && or ||
ReductionPtr handleReductionClause(const MatchMap& mmap) {

	auto fit = mmap.find("reduction");
	if(fit == mmap.end())
		return ReductionPtr();

	// we have a reduction
	// check the operator
	auto opIt = mmap.find("reduction_op");
	assert(opIt != mmap.end() && "Reduction clause doesn't contains an operator");
	const ValueList& opVar = opIt->second;
	assert(opVar.size() == 1);

	std::string* opStr = opVar.front()->get<std::string*>();
	assert(opStr && "Reduction clause with no operator");

	Reduction::Operator op;
	if(*opStr == "+")		op = Reduction::PLUS;
	else if(*opStr == "-")	op = Reduction::MINUS;
	else if(*opStr == "*")	op = Reduction::STAR;
	else if(*opStr == "&")	op = Reduction::AND;
	else if(*opStr == "|")	op = Reduction::OR;
	else if(*opStr == "^")	op = Reduction::XOR;
	else if(*opStr == "&&")	op = Reduction::LAND;
	else if(*opStr == "||")	op = Reduction::LOR;
	else assert(false && "Reduction operator not supported.");

	return std::make_shared<Reduction>(op, handleIdentifierList(mmap, "reduction"));
}

const clang::Expr* handleSingleExpression(const MatchMap& mmap, const std::string& key) {

	auto fit = mmap.find(key);
	if(fit == mmap.end()) { return NULL; }

	// we have an expression
	const ValueList& expr = fit->second;
	assert(expr.size() == 1);
	clang::Expr* collapseExpr = llvm::dyn_cast<clang::Expr>(expr.front()->get<clang::Stmt*>());
	assert(collapseExpr && "OpenMP collapse clause's expression is not of type clang::Expr");
	return collapseExpr;
}

// schedule( (static | dynamic | guided | atuo | runtime) (, chunk_size) )
SchedulePtr handleScheduleClause(const MatchMap& mmap) {

	auto fit = mmap.find("schedule");
	if(fit == mmap.end())
		return SchedulePtr();

	// we have a schedule clause
	const ValueList& kind = fit->second;
	assert(kind.size() == 1);
	std::string& kindStr = *kind.front()->get<std::string*>();

	Schedule::Kind k;
	if(kindStr == "static")
		k = Schedule::STATIC;
	else if (kindStr == "dynamic")
		k = Schedule::DYNAMIC;
	else if (kindStr == "guided")
		k = Schedule::GUIDED;
	else if (kindStr == "auto")
		k = Schedule::AUTO;
	else if (kindStr == "runtime")
		k = Schedule::RUNTIME;
	else
		assert(false && "Unsupported scheduling kind");

	// check for chunk_size expression
	const clang::Expr* chunkSize = handleSingleExpression(mmap, "chunk_size");
	return std::make_shared<Schedule>(k, chunkSize);
}

bool hasKeyword(const MatchMap& mmap, const std::string& key) {
	auto fit = mmap.find(key);
	return fit != mmap.end();
}

DefaultPtr handleDefaultClause(const MatchMap& mmap) {

	auto fit = mmap.find("default");
	if(fit == mmap.end())
		return DefaultPtr();

	// we have a schedule clause
	const ValueList& kind = fit->second;
	assert(kind.size() == 1);
	std::string& kindStr = *kind.front()->get<std::string*>();

	Default::Kind k;
	if(kindStr == "shared")
		k = Default::SHARED;
	else if(kindStr == "none")
		k = Default::NONE;
	else
		assert(false && "Unsupported default kind");

	return std::make_shared<Default>(k);
}


// if(scalar-expression)
// num_threads(integer-expression)
// default(shared | none)
// private(list)
// firstprivate(list)
// shared(list)
// copyin(list)
// reduction(operator: list)
AnnotationPtr OmpPragmaParallel::toAnnotation() const {
	const MatchMap& map = getMap();
	// check for if clause
	const clang::Expr*	ifClause = handleSingleExpression(map, "if");
	// check for num_threads clause
	const clang::Expr*	numThreadsClause = handleSingleExpression(map, "num_threads");
	// check for default clause
	DefaultPtr defaultClause = handleDefaultClause(map);
	// check for private clause
	VarListPtr privateClause = handleIdentifierList(map, "private");
	// check for firstprivate clause
	VarListPtr firstPrivateClause = handleIdentifierList(map, "firstprivate");
	// check for shared clause
	VarListPtr sharedClause = handleIdentifierList(map, "shared");
	// check for copyin clause
	VarListPtr copyinClause = handleIdentifierList(map, "copyin");
	// check for reduction clause
	ReductionPtr reductionClause = handleReductionClause(map);

	// check for 'for'
	if(hasKeyword(map, "for")) {
		// this is a parallel for
		VarListPtr lastPrivateClause = handleIdentifierList(map, "lastprivate");
		// check for schedule clause
		SchedulePtr scheduleClause = handleScheduleClause(map);
		// check for collapse cluase
		const clang::Expr*	collapseClause = handleSingleExpression(map, "collapse");
		// check for nowait keyword
		bool noWait = hasKeyword(map, "nowait");

		return std::make_shared<ParallelFor>(ifClause, numThreadsClause, 
				defaultClause, privateClause, firstPrivateClause, sharedClause, 
				copyinClause, reductionClause, lastPrivateClause, scheduleClause, 
				collapseClause, noWait);
	}

	// check for 'sections'
	if(hasKeyword(map, "sections")) {
		// this is a parallel for
		VarListPtr lastPrivateClause = handleIdentifierList(map, "lastprivate");
		// check for nowait keyword
		bool noWait = hasKeyword(map, "nowait");

		return std::make_shared<ParallelSections>(
			ifClause, numThreadsClause, defaultClause, privateClause,
					firstPrivateClause, sharedClause, copyinClause, 
					reductionClause, lastPrivateClause, noWait
		);
	}

	return std::make_shared<Parallel>(
			ifClause, numThreadsClause, defaultClause, privateClause,
					firstPrivateClause, sharedClause, copyinClause, reductionClause
	);

}

AnnotationPtr OmpPragmaFor::toAnnotation() const {
	const MatchMap& map = getMap();
	// check for private clause
	VarListPtr privateClause = handleIdentifierList(map, "private");
	// check for firstprivate clause
	VarListPtr firstPrivateClause = handleIdentifierList(map, "firstprivate");
	// check for lastprivate clause
	VarListPtr lastPrivateClause = handleIdentifierList(map, "lastprivate");
	// check for reduction clause
	ReductionPtr reductionClause = handleReductionClause(map);
	// check for schedule clause
	SchedulePtr scheduleClause = handleScheduleClause(map);
	// check for collapse cluase
	const clang::Expr*	collapseClause = handleSingleExpression(map, "collapse");
	// check for nowait keyword
	bool noWait = hasKeyword(map, "nowait");

	return std::make_shared<For>( privateClause, firstPrivateClause, lastPrivateClause,
								  reductionClause, scheduleClause, collapseClause, noWait );
}

// Translate a pragma omp section into a OmpSection annotation
// private(list)
// firstprivate(list)
// lastprivate(list)
// reduction(operator: list)
// nowait
AnnotationPtr OmpPragmaSections::toAnnotation() const {
	const MatchMap& map = getMap();
	// check for private clause
	VarListPtr privateClause = handleIdentifierList(map, "private");
	// check for firstprivate clause
	VarListPtr firstPrivateClause = handleIdentifierList(map, "firstprivate");
	// check for lastprivate clause
	VarListPtr lastPrivateClause = handleIdentifierList(map, "lastprivate");
	// check for reduction clause
	ReductionPtr reductionClause = handleReductionClause(map);
	// check for nowait keyword
	bool noWait = hasKeyword(map, "nowait");

	return std::make_shared<Sections>( privateClause, firstPrivateClause, 
			lastPrivateClause, reductionClause, noWait );
}

AnnotationPtr OmpPragmaSection::toAnnotation() const {
	return std::make_shared<Section>( );
}

// OmpSingle
// private(list)
// firstprivate(list)
// copyprivate(list)
// nowait
AnnotationPtr OmpPragmaSingle::toAnnotation() const {
	const MatchMap& map = getMap();
	// check for private clause
	VarListPtr privateClause = handleIdentifierList(map, "private");
	// check for firstprivate clause
	VarListPtr firstPrivateClause = handleIdentifierList(map, "firstprivate");
	// check for copyprivate clause
	VarListPtr copyPrivateClause = handleIdentifierList(map, "copyprivate");
	// check for nowait keyword
	bool noWait = hasKeyword(map, "nowait");

	return std::make_shared<Single>( privateClause, firstPrivateClause, 
			copyPrivateClause, noWait );
}

// if(scalar-expression)
// untied
// default(shared | none)
// private(list)
// firstprivate(list)
// shared(list)
AnnotationPtr OmpPragmaTask::toAnnotation() const {
	const MatchMap& map = getMap();
	// check for if clause
	const clang::Expr*	ifClause = handleSingleExpression(map, "if");
	// check for nowait keyword
	bool untied = hasKeyword(map, "untied");
	// check for default clause
	DefaultPtr defaultClause = handleDefaultClause(map);
	// check for private clause
	VarListPtr privateClause = handleIdentifierList(map, "private");
	// check for firstprivate clause
	VarListPtr firstPrivateClause = handleIdentifierList(map, "firstprivate");
	// check for shared clause
	VarListPtr sharedClause = handleIdentifierList(map, "shared");
	// We need to check if the
	return make_shared<Task>( ifClause, 
				untied, defaultClause, privateClause, 
				firstPrivateClause, sharedClause 
			);
}

AnnotationPtr OmpPragmaMaster::toAnnotation() const {
	return std::make_shared<Master>( );
}

AnnotationPtr OmpPragmaCritical::toAnnotation() const {
	const MatchMap& map = getMap();

	std::string name;
	// checking region name (if existing)
	auto fit = map.find("critical");
	if(fit != map.end()) {
		const ValueList& vars = fit->second;
		assert(vars.size() == 1 && "Critical region has multiple names");
		name = *vars.front()->get<std::string*>();
	}

	return std::make_shared<Critical>( name );
}

AnnotationPtr OmpPragmaBarrier::toAnnotation() const {
	std::cout << "Barrier" << std::endl;
	return std::make_shared<Barrier>( );
}

AnnotationPtr OmpPragmaTaskWait::toAnnotation() const {
	return std::make_shared<TaskWait>( );
}

AnnotationPtr OmpPragmaAtomic::toAnnotation() const {
	return std::make_shared<Atomic>( );
}

AnnotationPtr OmpPragmaFlush::toAnnotation() const {
	// check for flush identifier list
	VarListPtr flushList = handleIdentifierList(getMap(), "flush");
	return std::make_shared<Flush>( flushList );
}

AnnotationPtr OmpPragmaOrdered::toAnnotation() const {
	return std::make_shared<Ordered>( );
}

AnnotationPtr OmpPragmaThreadPrivate::toAnnotation() const {
	return std::make_shared<ThreadPrivate>();
}

} // end anonymous namespace

