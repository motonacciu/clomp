project(clomp CXX)

# States that CMake required version must be >= 2.6
cmake_minimum_required(VERSION 2.6)

enable_testing()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag( -std=c++0x CXX0X_Support )
if(CXX0X_Support)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g")
else()
	message( "WARNING: --std=c++0x not supported by your compiler!" )
endif()

set(LLVM_HOME /media/data/software/llvm-latest)

include_directories( 
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${LLVM_HOME}/include  )

set(clang_LList
	clangBasic clangSema clangIndex clangDriver clangAST
    clangRewrite clangAnalysis clangLex clangFrontend clangFrontendTool 
    clangParse clangSerialization clang
)

set(LINKING_TYPE SHARED)

find_library(llvm_LIB  NAMES LLVM-3.0 HINTS ${LLVM_HOME}/lib)
set(clang_LList clang ${clang_LList})

# Find (all?) clang libraries
foreach (name ${clang_LList})
	find_library(clang_${name}_LIB  NAMES ${name} HINTS ${LLVM_HOME}/lib)
    set(clang_LIBs ${clang_${name}_LIB} ${clang_LIBs})
endforeach(name)

find_library(pthread_LIB pthread)

# The macro below forces the build directory to be different from source directory:
# include(MacroOutOfSourceBuild)
# macro_ensure_out_of_source_build("${PROJECT_NAME} requires an out of source build.")

file(GLOB_RECURSE clomp_SOURCES  src/*.cpp )
add_library (Clomp ${clomp_SOURCES})

# Create an executable file called clomp from sources:
add_executable(clomp src/main.cxx)

target_link_libraries(clomp Clomp ${clang_LIBs} ${pthread_LIB} ${llvm_LIB})

add_executable(ut_pragma_matcher test/pragma_matcher_test.cc)
target_link_libraries(ut_pragma_matcher Clomp ${clang_LIBs} ${pthread_LIB} ${llvm_LIB}
	gtest gtest_main)

