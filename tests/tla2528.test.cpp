// Copyright 2024 Khalil Estell
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <libhal-expander/tla2528.hpp>
#include <libhal-expander/tla2528_adapters.hpp>

#include <boost/ut.hpp>

namespace hal::expander {
boost::ut::suite test_tla2528 = []() {
  using namespace boost::ut;
  using namespace std::literals;

  "tla2528::tla2528()"_test = []() {
    // Setup
    // Exercise
    // Verify
  };
};
}  // namespace hal::expander
