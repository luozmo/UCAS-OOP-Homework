#include "cache_stats.h"

cache_stats operator-(cache_stats lhs, cache_stats rhs)
{
  cache_stats result;
  result.pf_requested = lhs.pf_requested - rhs.pf_requested;
  result.pf_issued = lhs.pf_issued - rhs.pf_issued;
  result.pf_useful = lhs.pf_useful - rhs.pf_useful;
  result.pf_useless = lhs.pf_useless - rhs.pf_useless;
  result.pf_fill = lhs.pf_fill - rhs.pf_fill;

  // 预取提前量统计也按相同规则做差分。
  result.pf_timing_fills = lhs.pf_timing_fills - rhs.pf_timing_fills;
  result.pf_timing_timely = lhs.pf_timing_timely - rhs.pf_timing_timely;
  result.pf_timing_late = lhs.pf_timing_late - rhs.pf_timing_late;
  result.pf_timing_expired = lhs.pf_timing_expired - rhs.pf_timing_expired;
  result.pf_lead_time_sum_cycles = lhs.pf_lead_time_sum_cycles - rhs.pf_lead_time_sum_cycles;

  result.prefetch_source_count = lhs.prefetch_source_count;
  for (std::size_t source = 0; source < champsim::prefetching::max_prefetch_sources; ++source) {
    result.pf_source_issued.at(source) = lhs.pf_source_issued.at(source) - rhs.pf_source_issued.at(source);
    result.pf_source_useful.at(source) = lhs.pf_source_useful.at(source) - rhs.pf_source_useful.at(source);
    result.pf_source_useless.at(source) = lhs.pf_source_useless.at(source) - rhs.pf_source_useless.at(source);
    result.pf_source_timely.at(source) = lhs.pf_source_timely.at(source) - rhs.pf_source_timely.at(source);
    result.pf_source_late.at(source) = lhs.pf_source_late.at(source) - rhs.pf_source_late.at(source);
    result.pf_source_timing_fills.at(source) = lhs.pf_source_timing_fills.at(source) - rhs.pf_source_timing_fills.at(source);
    result.pf_source_expired.at(source) = lhs.pf_source_expired.at(source) - rhs.pf_source_expired.at(source);
  }

  result.hits = lhs.hits - rhs.hits;
  result.misses = lhs.misses - rhs.misses;

  result.total_miss_latency_cycles = lhs.total_miss_latency_cycles - rhs.total_miss_latency_cycles;
  return result;
}
