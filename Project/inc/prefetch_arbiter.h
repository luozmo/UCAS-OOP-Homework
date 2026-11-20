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

#ifndef PREFETCH_ARBITER_H
#define PREFETCH_ARBITER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "prefetch_arbiter_policy.h"
#include "prefetch_candidate.h"

namespace champsim::prefetching
{

enum class prefetch_arbiter_mode {
  none,
  round_robin,
};

enum class prefetch_decision {
  accepted,
  duplicate,
  budget_exhausted,
  invalid_source,
};

struct prefetch_arbiter_config {
  std::size_t filter_entries{64};
  std::size_t duplicate_window_cycles{256};
  std::size_t budget_per_window{8};
  std::size_t window_cycles{64};
  std::size_t source_count{1};
  std::string policy{"round_robin"};
};

struct prefetch_arbiter_stats {
  std::array<uint64_t, max_prefetch_sources> requested{};
  std::array<uint64_t, max_prefetch_sources> issued{};
  std::array<uint64_t, max_prefetch_sources> duplicate{};
  std::array<uint64_t, max_prefetch_sources> budget_dropped{};
  std::array<uint64_t, max_prefetch_sources> policy_dropped{};

  uint64_t total_requested{};
  uint64_t total_issued{};
  uint64_t total_duplicate{};
  uint64_t total_budget_dropped{};
  uint64_t total_policy_dropped{};
};

// This is the small, RTL-oriented arbiter used by the first ensemble version.
// It does not model confidence or dynamic scoring yet. It only performs:
//   1. direct-mapped duplicate filtering
//   2. a fixed per-window prefetch budget
//   3. per-source accounting
struct prefetch_source_update {
  uint64_t address{};
  uint64_t line_tag{};
  uint32_t source_mask{};
};

struct prefetch_arbitration_result {
  std::vector<prefetch_candidate> accepted;
  std::vector<prefetch_source_update> source_updates;
};

class prefetch_arbiter
{
public:
  explicit prefetch_arbiter(prefetch_arbiter_config config = {}, std::unique_ptr<prefetch_arbiter_policy> policy = nullptr);

  [[nodiscard]] prefetch_decision submit(const prefetch_candidate& candidate);
  [[nodiscard]] prefetch_arbitration_result arbitrate(const std::vector<prefetch_candidate>& candidates,
                                                       const prefetch_feedback& feedback = {});
  [[nodiscard]] const prefetch_arbiter_stats& stats() const;
  [[nodiscard]] std::size_t remaining_budget() const;
  [[nodiscard]] const prefetch_arbiter_policy& policy() const;
  void reset_stats();

private:
  struct filter_entry {
    uint64_t line_tag{};
    uint64_t last_seen_cycle{};
    uint32_t source_mask{};
    bool valid{};
  };

  void start_new_window(uint64_t cycle);
  [[nodiscard]] prefetch_decision submit_internal(const prefetch_candidate& candidate);
  [[nodiscard]] std::size_t filter_index(uint64_t line_tag) const;
  static uint32_t source_bit(std::size_t source_id);

  prefetch_arbiter_config config_;
  std::vector<filter_entry> filter_;
  std::size_t remaining_budget_{};
  uint64_t window_start_cycle_{};
  bool window_started_{};
  prefetch_arbiter_stats stats_{};
  std::unique_ptr<prefetch_arbiter_policy> policy_;
};

} // namespace champsim::prefetching

#endif
