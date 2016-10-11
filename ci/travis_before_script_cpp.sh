#!/usr/bin/env bash

#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License. See accompanying LICENSE file.


set -ex

source $TRAVIS_BUILD_DIR/ci/travis_install_conda.sh
conda install -y --channel apache/channel/dev parquet-cpp
export PARQUET_HOME=$MINICONDA

: ${CPP_BUILD_DIR=$TRAVIS_BUILD_DIR/cpp-build}

mkdir $CPP_BUILD_DIR
pushd $CPP_BUILD_DIR

CPP_DIR=$TRAVIS_BUILD_DIR/cpp

: ${ARROW_CPP_INSTALL=$TRAVIS_BUILD_DIR/cpp-install}

CMAKE_COMMON_FLAGS="\
-DARROW_BUILD_BENCHMARKS=ON \
-DARROW_PARQUET=OFF \
-DARROW_HDFS=ON \
-DCMAKE_INSTALL_PREFIX=$ARROW_CPP_INSTALL"

if [ $TRAVIS_OS_NAME == "linux" ]; then
    cmake -DARROW_TEST_MEMCHECK=on \
          $CMAKE_COMMON_FLAGS \
          -DCMAKE_CXX_FLAGS="-Werror" \
          $CPP_DIR
else
    cmake $CMAKE_COMMON_FLAGS \
          -DCMAKE_CXX_FLAGS="-Werror" \
          $CPP_DIR
fi

make -j4
make install

popd
