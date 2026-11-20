#ifndef CACHE_STATS_H
#define CACHE_STATS_H

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "channel.h"
#include "event_counter.h"
#include "prefetch_arbiter_policy.h"

struct cache_stats {
  std::string name;
  // prefetch stats
  uint64_t pf_requested = 0;
  uint64_t pf_issued = 0;
  uint64_t pf_useful = 0;
  uint64_t pf_useless = 0;
  uint64_t pf_fill = 0;

  // 以下字段用于“预取提前量”实验指标。
  // 它们不会改变缓存行为，只记录预取是否足够早地被 Demand 使用。
  uint64_t pf_timing_fills = 0;      // 已完成的、受跟踪的预取填充次数
  uint64_t pf_timing_timely = 0;     // Demand 在预取填充之后到达的次数
  uint64_t pf_timing_late = 0;       // Demand 在预取填充之前到达的次数
  uint64_t pf_timing_expired = 0;    // 预取在被使用前被驱逐或失效的次数
  int64_t pf_lead_time_sum_cycles = 0; // 所有 lead time 的累加值，单位为核心周期

  // Per-prefetcher feedback used by the arbiter policy.
  std::size_t prefetch_source_count{};
  std::array<uint64_t, champsim::prefetching::max_prefetch_sources> pf_source_issued{};
  std::array<uint64_t, champsim::prefetching::max_prefetch_sources> pf_source_useful{};
  std::array<uint64_t, champsim::prefetching::max_prefetch_sources> pf_source_useless{};
  std::array<uint64_t, champsim::prefetching::max_prefetch_sources> pf_source_timing_fills{};
  std::array<uint64_t, champsim::prefetching::max_prefetch_sources> pf_source_timely{};
  std::array<uint64_t, champsim::prefetching::max_prefetch_sources> pf_source_late{};
  std::array<uint64_t, champsim::prefetching::max_prefetch_sources> pf_source_expired{};

  champsim::stats::event_counter<std::pair<access_type, std::remove_cv_t<decltype(NUM_CPUS)>>> hits = {};
  champsim::stats::event_counter<std::pair<access_type, std::remove_cv_t<decltype(NUM_CPUS)>>> misses = {};
  champsim::stats::event_counter<std::pair<access_type, std::remove_cv_t<decltype(NUM_CPUS)>>> miss_merge = {};
  champsim::stats::event_counter<std::pair<access_type, std::remove_cv_t<decltype(NUM_CPUS)>>> fill = {};

  long total_miss_latency_cycles{};
};

cache_stats operator-(cache_stats lhs, cache_stats rhs);

#endif
