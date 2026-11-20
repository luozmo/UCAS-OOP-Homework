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

#include "prefetch_arbiter.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace champsim::prefetching
{

round_robin_prefetch_arbiter_policy::round_robin_prefetch_arbiter_policy(std::size_t source_count) : source_count_(source_count) {}

std::string_view round_robin_prefetch_arbiter_policy::name() const { return "round_robin"; }

std::vector<std::size_t> round_robin_prefetch_arbiter_policy::rank(const std::vector<prefetch_candidate>& candidates,
                                                                   const prefetch_arbiter_context& context)
{
  const auto source_count = std::min(source_count_ == 0 ? context.source_count : source_count_, context.source_count);
  std::vector<std::vector<std::size_t>> by_source(source_count);
  for (std::size_t index = 0; index < candidates.size(); ++index) {
    if (candidates.at(index).source_id < source_count) {
      by_source.at(candidates.at(index).source_id).push_back(index);
    }
  }

  std::vector<std::size_t> order{};
  order.reserve(candidates.size());
  for (std::size_t offset = 0; offset < source_count; ++offset) {
    const auto source = (next_source_ + offset) % source_count;
    order.insert(order.end(), by_source.at(source).begin(), by_source.at(source).end());
  }

  if (source_count > 0) {
    next_source_ = (next_source_ + 1) % source_count;
  }
  return order;
}

fixed_priority_prefetch_arbiter_policy::fixed_priority_prefetch_arbiter_policy(std::vector<std::size_t> source_order)
    : source_order_(std::move(source_order))
{
}

std::string_view fixed_priority_prefetch_arbiter_policy::name() const { return "fixed_priority"; }

std::vector<std::size_t> fixed_priority_prefetch_arbiter_policy::rank(const std::vector<prefetch_candidate>& candidates,
                                                                      const prefetch_arbiter_context& context)
{
  const auto source_count = context.source_count;
  auto order_config = source_order_;
  if (order_config.empty()) {
    order_config.resize(source_count);
    for (std::size_t source = 0; source < source_count; ++source) {
      order_config.at(source) = source;
    }
  }

  std::vector<std::size_t> source_rank(max_prefetch_sources, std::numeric_limits<std::size_t>::max());
  for (std::size_t rank_index = 0; rank_index < order_config.size(); ++rank_index) {
    if (order_config.at(rank_index) < max_prefetch_sources) {
      source_rank.at(order_config.at(rank_index)) = rank_index;
    }
  }

  std::vector<std::size_t> order(candidates.size());
  for (std::size_t index = 0; index < candidates.size(); ++index) {
    order.at(index) = index;
  }
  std::stable_sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
    const auto lhs_source = candidates.at(lhs).source_id;
    const auto rhs_source = candidates.at(rhs).source_id;
    const auto lhs_rank = lhs_source < source_rank.size() ? source_rank.at(lhs_source) : std::numeric_limits<std::size_t>::max();
    const auto rhs_rank = rhs_source < source_rank.size() ? source_rank.at(rhs_source) : std::numeric_limits<std::size_t>::max();
    return lhs_rank < rhs_rank;
  });
  return order;
}

std::string_view timeliness_aware_prefetch_arbiter_policy::name() const { return "timeliness_aware"; }

std::vector<std::size_t> timeliness_aware_prefetch_arbiter_policy::rank(const std::vector<prefetch_candidate>& candidates,
                                                                        const prefetch_arbiter_context& context)
{
  std::vector<std::size_t> order(candidates.size());
  for (std::size_t index = 0; index < candidates.size(); ++index) {
    order.at(index) = index;
  }

  auto score = [&](std::size_t source_id) {
    if (source_id >= context.feedback.sources.size()) {
      return 0.0;
    }
    const auto& feedback = context.feedback.sources.at(source_id);
    const auto timely = static_cast<double>(feedback.timely);
    const auto late = static_cast<double>(feedback.late);
    const auto expired = static_cast<double>(feedback.expired);
    return (timely + 1.0) / (timely + late + expired + 1.0);
  };

  std::stable_sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
    return score(candidates.at(lhs).source_id) > score(candidates.at(rhs).source_id);
  });
  return order;
}

std::unique_ptr<prefetch_arbiter_policy> make_prefetch_arbiter_policy(std::string_view name, std::size_t source_count)
{
  if (name == "round_robin") {
    return std::make_unique<round_robin_prefetch_arbiter_policy>(source_count);
  }
  if (name == "fixed_priority") {
    std::vector<std::size_t> order(source_count);
    for (std::size_t source = 0; source < source_count; ++source) {
      order.at(source) = source;
    }
    return std::make_unique<fixed_priority_prefetch_arbiter_policy>(std::move(order));
  }
  if (name == "timeliness_aware") {
    return std::make_unique<timeliness_aware_prefetch_arbiter_policy>();
  }
  throw std::invalid_argument{"unknown prefetch arbiter policy: " + std::string{name}};
}

prefetch_arbiter::prefetch_arbiter(prefetch_arbiter_config config, std::unique_ptr<prefetch_arbiter_policy> policy)
    : config_(std::move(config)), filter_(config_.filter_entries), remaining_budget_(config_.budget_per_window),
      policy_(policy ? std::move(policy) : make_prefetch_arbiter_policy(config_.policy, config_.source_count))
{
  if (config_.filter_entries == 0) {
    throw std::invalid_argument{"prefetch arbiter filter must have at least one entry"};
  }
  if (config_.window_cycles == 0) {
    throw std::invalid_argument{"prefetch arbiter window must be at least one cycle"};
  }
  if (config_.duplicate_window_cycles == 0) {
    throw std::invalid_argument{"prefetch duplicate window must be at least one cycle"};
  }
  if (config_.source_count == 0 || config_.source_count > max_prefetch_sources) {
    throw std::invalid_argument{"prefetch arbiter source count is out of range"};
  }
}

prefetch_decision prefetch_arbiter::submit(const prefetch_candidate& candidate)
{
  if (candidate.source_id >= config_.source_count) {
    return prefetch_decision::invalid_source;
  }

  ++stats_.total_requested;
  ++stats_.requested.at(candidate.source_id);
  return submit_internal(candidate);
}

prefetch_arbitration_result prefetch_arbiter::arbitrate(const std::vector<prefetch_candidate>& candidates, const prefetch_feedback& feedback)
{
  prefetch_arbitration_result result{};
  if (candidates.empty()) {
    return result;
  }

  const auto cycle = candidates.front().cycle;
  if (!window_started_ || cycle >= window_start_cycle_ + config_.window_cycles) {
    start_new_window(cycle);
  }

  for (const auto& candidate : candidates) {
    if (candidate.source_id < config_.source_count) {
      ++stats_.total_requested;
      ++stats_.requested.at(candidate.source_id);
    } else {
      ++stats_.total_policy_dropped;
    }
  }

  const auto order = policy_->rank(candidates, prefetch_arbiter_context{cycle, remaining_budget_, config_.source_count, feedback});
  std::vector<bool> considered(candidates.size(), false);
  for (const auto index : order) {
    if (index >= candidates.size() || considered.at(index)) {
      continue;
    }
    considered.at(index) = true;
    const auto candidate = candidates.at(index);
    if (candidate.source_id >= config_.source_count) {
      continue;
    }
    const auto decision = submit_internal(candidate);
    if (decision == prefetch_decision::accepted) {
      result.accepted.push_back(candidate);
    } else if (decision == prefetch_decision::duplicate) {
      const auto& entry = filter_.at(filter_index(candidate.line_tag));
      result.source_updates.push_back({candidate.address, candidate.line_tag, entry.source_mask});
    }
  }

  for (std::size_t index = 0; index < candidates.size(); ++index) {
    if (considered.at(index) || candidates.at(index).source_id >= config_.source_count) {
      continue;
    }
    ++stats_.total_policy_dropped;
    ++stats_.policy_dropped.at(candidates.at(index).source_id);
  }

  return result;
}

const prefetch_arbiter_stats& prefetch_arbiter::stats() const { return stats_; }

std::size_t prefetch_arbiter::remaining_budget() const { return remaining_budget_; }

const prefetch_arbiter_policy& prefetch_arbiter::policy() const { return *policy_; }

void prefetch_arbiter::reset_stats() { stats_ = {}; }

prefetch_decision prefetch_arbiter::submit_internal(const prefetch_candidate& candidate)
{
  if (!window_started_ || candidate.cycle >= window_start_cycle_ + config_.window_cycles) {
    start_new_window(candidate.cycle);
  }

  auto& entry = filter_.at(filter_index(candidate.line_tag));
  const bool duplicate = entry.valid && entry.line_tag == candidate.line_tag
                         && candidate.cycle <= entry.last_seen_cycle + config_.duplicate_window_cycles;
  if (duplicate) {
    entry.last_seen_cycle = candidate.cycle;
    entry.source_mask |= source_bit(candidate.source_id);
    ++stats_.total_duplicate;
    ++stats_.duplicate.at(candidate.source_id);
    return prefetch_decision::duplicate;
  }

  if (remaining_budget_ == 0) {
    ++stats_.total_budget_dropped;
    ++stats_.budget_dropped.at(candidate.source_id);
    return prefetch_decision::budget_exhausted;
  }

  --remaining_budget_;
  entry = filter_entry{candidate.line_tag, candidate.cycle, source_bit(candidate.source_id), true};
  ++stats_.total_issued;
  ++stats_.issued.at(candidate.source_id);
  return prefetch_decision::accepted;
}

void prefetch_arbiter::start_new_window(uint64_t cycle)
{
  window_started_ = true;
  window_start_cycle_ = cycle;
  remaining_budget_ = config_.budget_per_window;
}

std::size_t prefetch_arbiter::filter_index(uint64_t line_tag) const { return static_cast<std::size_t>(line_tag % config_.filter_entries); }

uint32_t prefetch_arbiter::source_bit(std::size_t source_id) { return uint32_t{1} << source_id; }

} // namespace champsim::prefetching
