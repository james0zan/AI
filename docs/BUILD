===========================
Caveats
===========================

This software has only been tested on Ubuntu-12.04.3-amd64.
Some features, such as the `__thread` thread-local storage, 
may not be supported in the other platforms.

In order to implement the Recorder data structure defined in Section 3.2,
we use a concurrent hash map with fine-grained locks.
With the current configuration, it will take about 2 gigabytes memory.
However, we do not need to use such a big map in the most cases (except `performance/fft`).
Thus you can reduce the size of it by changing the row 137 of `ai/ai-rt/ai_rtl.hpp`
from:

    Recorder->Init(25);

to:

    Recorder->Init(20);

A Recorder initialized by N will have 2^N slots.

===========================
Build AI
===========================

---------------------------
Environment Variables
---------------------------

First, you need to set several environment variables.
    
    #!/usr/bin/env bash
    export AI_ARCHIVE_DIR=/path/to/the/directory/of/the/decompressed/ai/archive
    export AI_HOME=$AI_ARCHIVE_DIR/ai

---------------------------
Installing Dependencies
---------------------------

The following packages are needed for building AI and the demos:

    #!/usr/bin/env bash
    apt-get install -q -y --force-yes build-essential
    apt-get install -q -y --force-yes make
    apt-get install -q -y --force-yes python-dev
    apt-get install -q -y --force-yes pkg-config
    apt-get install -q -y --force-yes libxml2-dev
    apt-get install -q -y --force-yes libbz2-dev
    apt-get install -q -y --force-yes m4
    apt-get install -q -y --force-yes autoconf automake libtool
    apt-get install -q -y --force-yes gettext

---------------------------
Build and Install CMake
---------------------------

The version of CMake in Ubuntu's source is too old.
Thus we need to build and install the CMake from source code.

    #!/usr/bin/env bash
    cd $AI_ARCHIVE_DIR
    tar -xzf cmake-2.8.12.tar.gz
    cd cmake-2.8.12/
    ./configure 
    make
    make install

This will install CMake 2.8.12 to /usr/local/bin/cmake, etc.

---------------------------
Build LLVM & Clang
---------------------------

Build and install LLVM and Clang to `$AI_ARCHIVE_DIR/llvm/install` with *cmake*.

    #!/usr/bin/env bash
    cd $AI_ARCHIVE_DIR
    tar -xzf llvm-3.3.src.tar.gz 
    mv llvm-3.3.src llvm
    tar -xzf cfe-3.3.src.tar.gz 
    mv cfe-3.3.src llvm/tools/clang
    cd llvm
    mkdir install
    mkdir build && cd build
    CC=gcc CXX=g++ /usr/local/bin/cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_BUILD_TESTS=1 -DLLVM_REQUIRES_RTTI=1 -DCMAKE_INSTALL_PREFIX=$AI_ARCHIVE_DIR/llvm/install ..
    make
    make install

Set the environment variables:

    #!/usr/bin/env bash
    export LLVM_ROOT=$AI_ARCHIVE_DIR/llvm/install
    export PATH=$PATH:/$LLVM_ROOT/bin

---------------------------
Build AI
---------------------------

    cd $AI_HOME
    mkdir build && cd build
    cmake ..
    make
    export LD_LIBRARY_PATH=$AI_HOME/build/ai-rt/:$LD_LIBRARY_PATH
    export LIBRARY_PATH=$AI_HOME/build/ai-rt/:$LIBRARY_PATH

Check whether $PATH contains `$LLVM_ROOT/bin`, if cmake says that the LLVM package is not found.
