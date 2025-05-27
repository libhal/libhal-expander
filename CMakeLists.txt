# Copyright 2024 - 2025 Khalil Estell and the libhal contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 3.15)

project(libhal-expander LANGUAGES CXX)

libhal_test_and_make_library(
  LIBRARY_NAME libhal-expander

  SOURCES
  src/pca9685.cpp
  src/tla2528.cpp
  src/tla2528_adapters.cpp

  TEST_SOURCES
  tests/pca9685.test.cpp
  tests/tla2528.test.cpp
  tests/main.test.cpp
)
