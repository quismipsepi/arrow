#!/usr/bin/env bash

# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

set -ex

# Fail fast for code linting issues

if [ "$ARROW_CI_CPP_AFFECTED" != "0" ]; then
  mkdir $TRAVIS_BUILD_DIR/cpp/lint
  pushd $TRAVIS_BUILD_DIR/cpp/lint

  cmake ..
  make lint

  if [ "$ARROW_TRAVIS_CLANG_FORMAT" == "1" ]; then
    make check-format
  fi

  popd
fi


# Fail fast on style checks

if [ "$ARROW_CI_PYTHON_AFFECTED" != "0" ]; then
  sudo pip install -q flake8

  PYTHON_DIR=$TRAVIS_BUILD_DIR/python

  flake8 --count $PYTHON_DIR

  # Check Cython files with some checks turned off
  flake8 --count --config=$PYTHON_DIR/.flake8.cython \
         $PYTHON_DIR
fi
