# setup environment variables
. ./environment.setup

###########################################################################
#  instalation notes:  
#  ==================
#
#   make sure which llvm llvm-latest is pointing to
#   libLLVM-3.2.so may not exist after compilation    
#             create an alias to libLLVM-3.2svn.so in $PREFIX/llvm-3.2/libs
###########################################################################

VERSION=3.2

CURRENT=`pwd`

# download llvm 
echo "*****************************************"
echo "* Downloading current LLVM distribution *"
echo "*****************************************"
wget -nc http://llvm.org/releases/$VERSION/llvm-$VERSION.src.tar.gz 

RET=$?
if [ $RET -ne 0 ]; then
	exit $RET
fi

tar -xf llvm-$VERSION.src.tar.gz
# change dire into tools

echo "******************************************"
echo "* Downloading current CLANG distribution *"
echo "******************************************"

cd llvm-$VERSION.src/tools
wget -nc http://llvm.org/releases/$VERSION/clang-$VERSION.src.tar.gz 

RET=$?
if [ $RET -ne 0 ]; then
	exit $RET
fi

tar -xf clang-$VERSION.src.tar.gz
mv clang-$VERSION.src clang
rm -f clang-$VERSION.src.tar.gz
cd $CURRENT

echo "******************************************"
echo "* Downloading compiler RUNTIME support   *"
echo "******************************************"

cd llvm-$VERSION.src/projects
wget -nc http://llvm.org/releases/$VERSION/compiler-rt-$VERSION.src.tar.gz

RET=$?
if [ $RET -ne 0 ]; then
	exit $RET
fi

tar -xf compiler-rt-$VERSION.src.tar.gz
mv compiler-rt-$VERSION.src compiler-rt
rm -f compiler-rt-$VERSION.src.tar.gz
cd $CURRENT

echo "***********************************"
echo "* Applying insieme patch to CLANG *"
echo "***********************************"
cd llvm-$VERSION.src

patch -p1  < clomp-$VERSION.patch

RET=$?
if [ $RET -ne 0 ]; then
	exit $RET
fi

echo "*******************"
echo "* Compiling CLANG *"
echo "*******************"

mkdir llvm-build
cd llvm-build

$CURRENT/llvm-$VERSION.src/configure --prefix=$PREFIX/llvm-$VERSION --enable-shared=yes\
  	 --enable-assert=yes --enable-debug-runtime=no --enable-debug-symbols=no --enable-optimized=yes

make REQUIRES_RTTI=1 clang-only -j$SLOTS

# Check for failure
RET=$?
if [ $RET -ne 0 ]; then
	echo " compilation failed "
	exit $RET
fi

make clang-only install

cd ../
echo "****************************************"
echo "* Removing LLVM installation directory *"
echo "****************************************"
rm -Rf llvm-$VERSION.src*

ln -s $PREFIX/llvm-$VERSION/lib/libLLVM-3.2svn.so $PREFIX/llvm-$VERSION/lib/libLLVM-3.2.so

exit 0
