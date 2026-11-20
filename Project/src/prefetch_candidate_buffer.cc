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

#include "prefetch_candidate_buffer.h"

#include <algorithm>
#include <stdexcept>

namespace champsim::prefetching
{

prefetch_candidate_buffer::prefetch_candidate_buffer(prefetch_candidate_buffer_config config) : config_(config)
{
  if (config_.source_count == 0 || config_.source_count > max_prefetch_sources) {
    throw std::invalid_argument{"prefetch candidate buffer source count is out of range"};
  }
  if (config_.capacity_per_source == 0) {
    throw std::invalid_argument{"prefetch candidate buffer capacity must be positive"};
  }
}

bool prefetch_candidate_buffer::push(const prefetch_candidate& candidate)
{
  if (candidate.source_id >= config_.source_count) {
    ++stats_.invalid_source;
    return false;
  }

  auto& queue = queues_.at(candidate.source_id);
  if (queue.size() >= config_.capacity_per_source) {
    ++stats_.total_overflow;
    ++stats_.overflow.at(candidate.source_id);
    return false;
  }

  queue.push_back(candidate);
  ++stats_.total_enqueued;
  ++stats_.enqueued.at(candidate.source_id);
  return true;
}

std::optional<prefetch_candidate> prefetch_candidate_buffer::pop_round_robin()
{
  const auto source = next_nonempty_source();
  if (source == config_.source_count) {
    return std::nullopt;
  }

  auto& queue = queues_.at(source);
  auto candidate = queue.front();
  queue.pop_front();
  next_source_ = (source + 1) % config_.source_count;
  return candidate;
}

std::vector<prefetch_candidate> prefetch_candidate_buffer::drain_round_robin()
{
  std::vector<prefetch_candidate> drained{};
  while (auto candidate = pop_round_robin()) {
    drained.push_back(*candidate);
  }
  return drained;
}

void prefetch_candidate_buffer::clear()
{
  for (auto& queue : queues_) {
    queue.clear();
  }
  next_source_ = 0;
}

void prefetch_candidate_buffer::reset_stats() { stats_ = {}; }

bool prefetch_candidate_buffer::empty() const
{
  return std::all_of(std::begin(queues_), std::begin(queues_) + config_.source_count, [](const auto& queue) { return queue.empty(); });
}

std::size_t prefetch_candidate_buffer::size(std::size_t source_id) const
{
  if (source_id >= config_.source_count) {
    return 0;
  }
  return queues_.at(source_id).size();
}

const prefetch_candidate_buffer_stats& prefetch_candidate_buffer::stats() const { return stats_; }

std::size_t prefetch_candidate_buffer::next_nonempty_source() const
{
  for (std::size_t offset = 0; offset < config_.source_count; ++offset) {
    const auto source = (next_source_ + offset) % config_.source_count;
    if (!queues_.at(source).empty()) {
      return source;
    }
  }
  return config_.source_count;
}

} // namespace champsim::prefetching
