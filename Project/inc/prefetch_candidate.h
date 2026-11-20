/*
 * Copyright 2026 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PREFETCH_CANDIDATE_H
#define PREFETCH_CANDIDATE_H

#include <cstdint>

namespace champsim::prefetching
{

// A request proposed by one prefetcher before arbitration.
// The address is kept as raw bits so the arbiter can also be mapped to RTL.
struct prefetch_candidate {
  uint64_t address{};
  uint64_t line_tag{};
  uint8_t source_id{};
  bool fill_this_level{};
  uint32_t metadata{};
  uint64_t cycle{};
  uint32_t source_mask{};
};

} // namespace champsim::prefetching

#endif
