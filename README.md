# clomp


OpenMP support for LLVM/Clang compiler 

## Installation

Clomp is based on LLVM version 3.0 source. In order to work Clomp needs a patch to be applyed to the Clang
source, therefore it is not compatible with later versions of LLVM. 

In order to install Clomp the following libraries are needed:
* gcc compiler with C++0x support (version 4.5.x or later)
* cmake (http://www.cmake.org/)
* gtest (http://code.google.com/p/googletest/)
* LLVM-3.2-patch

### Install patched LLVM-3.2

In order to install the required patched version of LLVM a script is provided in the script folder called 
llvm-3.2-installer.sh

In order for the script to find patched, it needs to be executed from the script directory. 
```
cd script/
PREFIX=/my/llvm/installation/path sh llvm-3.2-installer.sh 
```
The script will download and install LLVM in the given PREFIX path. 

### Create Project Makefile

Clomp is based on cmake. In order to build the makefile for the entire project run the following command from 
the root folder:

```
LLVM_HOME=/my/llvm/installation/path GTEST_ROOT=/gtest/build/path cmake .
```

Make sure that the LLVM_HOME enviroment variable points to the previously installed patched version of LLVM 
and that the C/C++ compiler seen by cmake is a c++0x capable compiler. If the compiler is installed in a non-standard
location then provide the correct path to the cmake using the CC and CXX environment variables.

### Done
If the cmake run through without errors, then you are all set to compiler Clomp. Just run
````
make
````
and cross your fingers. Make file should produce two executables, one test case called ut_pragma_matcher
and a second executable called clomp. 

These are both programs to check basic functionalities of Clomp. Expecially the clomp executable can be used to 
list (on standard output) the list of OpenMP pragmas being recongnized from an input program. The program will 
print the location at which the pragmas are recognized and to which statements/declaration they are being applied. 

```
./clomp my_omp_test.c
```

Have fun and please contributed! 

## License
Clomp is released under the University of Illinois/NCSA Open Source License

## What is missing? 

Clomp takes care of parsing and attaching OpenMP pragmas to Clang's AST nodes. Clomp is currently uded in the Insieme
compiler (http://www.insieme-compiler.org) which maps OpenMP concepts into 
calls to a runtime designed for heterogenous systems. I was able to open the source of the OpenMP parser but not the 
runtime, furthermore I expected people to be more interested on having a runtime based on LLVM therefore I hope with 
this project to open the street to who-ever wants to provide OpenMP capabilities to LLVM. 
Drop me a line if you are interested in using Clomp. 

