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

#ifndef PREFETCH_CANDIDATE_BUFFER_H
#define PREFETCH_CANDIDATE_BUFFER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

#include "prefetch_arbiter.h"
#include "prefetch_candidate.h"

namespace champsim::prefetching
{

struct prefetch_candidate_buffer_config {
  std::size_t source_count{1};
  std::size_t capacity_per_source{8};
};

struct prefetch_candidate_buffer_stats {
  std::array<uint64_t, max_prefetch_sources> enqueued{};
  std::array<uint64_t, max_prefetch_sources> overflow{};
  uint64_t total_enqueued{};
  uint64_t total_overflow{};
  uint64_t invalid_source{};
};

// Collects candidates independently for each prefetcher. Prefetchers write to
// their own queue, and the cache drains queues in round-robin order after all
// prefetchers have observed the same pre-arbitration cache state.
class prefetch_candidate_buffer
{
public:
  explicit prefetch_candidate_buffer(prefetch_candidate_buffer_config config = {});

  [[nodiscard]] bool push(const prefetch_candidate& candidate);
  [[nodiscard]] std::optional<prefetch_candidate> pop_round_robin();
  [[nodiscard]] std::vector<prefetch_candidate> drain_round_robin();
  void clear();
  void reset_stats();

  [[nodiscard]] bool empty() const;
  [[nodiscard]] std::size_t size(std::size_t source_id) const;
  [[nodiscard]] const prefetch_candidate_buffer_stats& stats() const;

private:
  [[nodiscard]] std::size_t next_nonempty_source() const;

  prefetch_candidate_buffer_config config_;
  std::array<std::deque<prefetch_candidate>, max_prefetch_sources> queues_{};
  std::size_t next_source_{};
  prefetch_candidate_buffer_stats stats_{};
};

} // namespace champsim::prefetching

#endif
