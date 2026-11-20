#include "builtin_metrics.h"

namespace champsim::metrics
{
metric_registry make_builtin_metric_registry()
{
  metric_registry registry{};

  // CPU 原始指标。
  registry.add({"instructions", "count", metric_kind::raw, std::nullopt, {}, "Retired instruction count"});
  registry.add({"cycles", "core_cycles", metric_kind::raw, std::nullopt, {}, "Simulation cycle count"});

  // CPU 派生指标。
  registry.add({"ipc", "instructions_per_cycle", metric_kind::derived, "instructions / cycles", {}, "Instructions retired per cycle"});

  // Cache 原始指标。
  // access_type 是维度，例如 LOAD、WRITE、PREFETCH。
  registry.add({"cache_hit", "count", metric_kind::raw, std::nullopt, {"access_type"}, "Cache hit count"});
  registry.add({"cache_miss", "count", metric_kind::raw, std::nullopt, {"access_type"}, "Cache miss count"});
  registry.add({"cache_miss_merge", "count", metric_kind::raw, std::nullopt, {"access_type"}, "Merged cache miss count"});
  registry.add({"cache_fill", "count", metric_kind::raw, std::nullopt, {"access_type"}, "Cache fill count"});

  registry.add({"prefetch_requested", "count", metric_kind::raw, std::nullopt, {}, "Prefetch request count"});
  registry.add({"prefetch_issued", "count", metric_kind::raw, std::nullopt, {}, "Issued prefetch count"});
  registry.add({"prefetch_useful", "count", metric_kind::raw, std::nullopt, {}, "Useful prefetch count"});
  registry.add({"prefetch_useless", "count", metric_kind::raw, std::nullopt, {}, "Useless prefetch count"});
  registry.add({"prefetch_fill", "count", metric_kind::raw, std::nullopt, {}, "Prefetch fill count"});

  // Per-prefetcher feedback. prefetcher_id is the configured source order.
  registry.add({"prefetch_source_issued", "count", metric_kind::raw, std::nullopt, {"prefetcher_id"}, "Issued prefetches attributed to a source"});
  registry.add({"prefetch_source_useful", "count", metric_kind::raw, std::nullopt, {"prefetcher_id"}, "Useful prefetches attributed to a source"});
  registry.add({"prefetch_source_useless", "count", metric_kind::raw, std::nullopt, {"prefetcher_id"}, "Useless prefetches attributed to a source"});
  registry.add({"prefetch_source_timely", "count", metric_kind::raw, std::nullopt, {"prefetcher_id"}, "Timely prefetches attributed to a source"});
  registry.add({"prefetch_source_late", "count", metric_kind::raw, std::nullopt, {"prefetcher_id"}, "Late prefetches attributed to a source"});
  registry.add({"prefetch_source_expired", "count", metric_kind::raw, std::nullopt, {"prefetcher_id"}, "Expired prefetches attributed to a source"});
  registry.add({"prefetch_source_timing_fills", "count", metric_kind::raw, std::nullopt, {"prefetcher_id"}, "Tracked prefetch fills attributed to a source"});

  // 预取提前量相关的原始计数。
  registry.add({"prefetch_timing_fills", "count", metric_kind::raw, std::nullopt, {}, "Tracked prefetch fills"});
  registry.add({"prefetch_timing_timely", "count", metric_kind::raw, std::nullopt, {}, "Prefetches used after they were filled"});
  registry.add({"prefetch_timing_late", "count", metric_kind::raw, std::nullopt, {}, "Prefetches whose demand arrived before fill completion"});
  registry.add({"prefetch_timing_expired", "count", metric_kind::raw, std::nullopt, {}, "Prefetches evicted or invalidated before first use"});
  registry.add({"prefetch_lead_time_sum_cycles", "core_cycles", metric_kind::raw, std::nullopt, {}, "Sum of prefetch lead times in core cycles"});

  // Cache 派生指标。
  registry.add({"prefetch_accuracy", "ratio", metric_kind::derived, "prefetch_useful / prefetch_issued", {}, "Fraction of issued prefetches that were useful"});
  registry.add({"prefetch_coverage", "ratio", metric_kind::derived, "prefetch_useful / demand_misses", {}, "Fraction of demand misses covered by useful prefetches"});
  registry.add({"cache_average_miss_latency", "core_cycles", metric_kind::derived, "total_miss_latency_cycles / demand_fills", {}, "Average cache miss latency"});
  registry.add({"prefetch_mean_lead_time", "core_cycles", metric_kind::derived, "prefetch_lead_time_sum_cycles / measured_prefetches", {}, "Average lead time of measured prefetches"});
  registry.add({"prefetch_timely_rate", "ratio", metric_kind::derived, "prefetch_timing_timely / measured_prefetches", {}, "Fraction of measured prefetches that were timely"});
  registry.add({"prefetch_late_rate", "ratio", metric_kind::derived, "prefetch_timing_late / measured_prefetches", {}, "Fraction of measured prefetches that were late"});
  registry.add({"prefetch_expiration_rate", "ratio", metric_kind::derived, "prefetch_timing_expired / prefetch_timing_fills", {}, "Fraction of tracked prefetches that expired before use"});
  registry.add({"prefetch_source_accuracy", "ratio", metric_kind::derived,
                "prefetch_source_useful / prefetch_source_issued", {"prefetcher_id"}, "Useful fraction for one prefetcher source"});
  registry.add({"prefetch_source_timely_rate", "ratio", metric_kind::derived,
                "prefetch_source_timely / (prefetch_source_timely + prefetch_source_late)", {"prefetcher_id"}, "Timely fraction for one prefetcher source"});
  registry.add({"prefetch_source_expiration_rate", "ratio", metric_kind::derived,
                "prefetch_source_expired / prefetch_source_timing_fills", {"prefetcher_id"}, "Expiration fraction for one prefetcher source"});

  // DRAM 原始指标。
  registry.add({"dram_rq_row_buffer_hit", "count", metric_kind::raw, std::nullopt, {}, "DRAM read queue row buffer hits"});
  registry.add({"dram_rq_row_buffer_miss", "count", metric_kind::raw, std::nullopt, {}, "DRAM read queue row buffer misses"});
  registry.add({"dram_wq_row_buffer_hit", "count", metric_kind::raw, std::nullopt, {}, "DRAM write queue row buffer hits"});
  registry.add({"dram_wq_row_buffer_miss", "count", metric_kind::raw, std::nullopt, {}, "DRAM write queue row buffer misses"});
  registry.add({"dram_wq_full", "count", metric_kind::raw, std::nullopt, {}, "DRAM write queue full events"});
  registry.add({"dram_refreshes_issued", "count", metric_kind::raw, std::nullopt, {}, "DRAM refresh count"});

  // DRAM 派生指标。
  registry.add({"dram_avg_dbus_congested_cycle", "cycles", metric_kind::derived, "dbus_cycle_congested / dbus_count_congested", {}, "Average data bus congested cycles"});

  return registry;
}
} // namespace champsim::metrics

