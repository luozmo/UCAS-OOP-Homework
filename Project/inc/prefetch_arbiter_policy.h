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

#ifndef PREFETCH_ARBITER_POLICY_H
#define PREFETCH_ARBITER_POLICY_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "prefetch_candidate.h"

namespace champsim::prefetching
{

inline constexpr std::size_t max_prefetch_sources = 8;

struct prefetch_source_feedback {
  uint64_t issued{};
  uint64_t useful{};
  uint64_t timely{};
  uint64_t late{};
  uint64_t expired{};
};

struct prefetch_feedback {
  std::array<prefetch_source_feedback, max_prefetch_sources> sources{};
};

struct prefetch_arbiter_context {
  uint64_t cycle{};
  std::size_t remaining_budget{};
  std::size_t source_count{};
  prefetch_feedback feedback{};
};

// A policy only ranks candidates. The arbiter still applies duplicate
// filtering, budget limits, and accounting after the ranking is returned.
class prefetch_arbiter_policy
{
public:
  virtual ~prefetch_arbiter_policy() = default;
  [[nodiscard]] virtual std::string_view name() const = 0;
  virtual std::vector<std::size_t> rank(const std::vector<prefetch_candidate>& candidates, const prefetch_arbiter_context& context) = 0;
};

class round_robin_prefetch_arbiter_policy final : public prefetch_arbiter_policy
{
public:
  explicit round_robin_prefetch_arbiter_policy(std::size_t source_count = 1);

  [[nodiscard]] std::string_view name() const override;
  std::vector<std::size_t> rank(const std::vector<prefetch_candidate>& candidates, const prefetch_arbiter_context& context) override;

private:
  std::size_t source_count_{};
  std::size_t next_source_{};
};

class fixed_priority_prefetch_arbiter_policy final : public prefetch_arbiter_policy
{
public:
  explicit fixed_priority_prefetch_arbiter_policy(std::vector<std::size_t> source_order = {});

  [[nodiscard]] std::string_view name() const override;
  std::vector<std::size_t> rank(const std::vector<prefetch_candidate>& candidates, const prefetch_arbiter_context& context) override;

private:
  std::vector<std::size_t> source_order_{};
};

class timeliness_aware_prefetch_arbiter_policy final : public prefetch_arbiter_policy
{
public:
  [[nodiscard]] std::string_view name() const override;
  std::vector<std::size_t> rank(const std::vector<prefetch_candidate>& candidates, const prefetch_arbiter_context& context) override;
};

// Users can extend this factory with their own policy implementation or pass a
// policy object directly to prefetch_arbiter's constructor.
[[nodiscard]] std::unique_ptr<prefetch_arbiter_policy> make_prefetch_arbiter_policy(std::string_view name, std::size_t source_count = 1);

} // namespace champsim::prefetching

#endif
