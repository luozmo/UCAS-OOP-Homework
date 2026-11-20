#include "phase_metric_extractor.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

#include "access_type.h"

namespace
{
std::string access_type_name(access_type type) { return std::string{access_type_names.at(champsim::to_underlying(type))}; }

std::int64_t as_count(std::uint64_t value) { return static_cast<std::int64_t>(value); }
} // namespace

namespace champsim::metrics
{
phase_metric_extractor::phase_metric_extractor(const metric_registry& registry, metric_sink& sink) : registry_(registry), sink_(sink) {}

void phase_metric_extractor::extract(const champsim::phase_stats& stats, std::string_view run_id) const
{
  // phase_stats 同时保存 sim 和 roi 两组统计。
  // 我们分别输出，phase 字段用于区分它们。
  extract_cpu(stats.sim_cpu_stats, "sim", run_id);
  extract_cpu(stats.roi_cpu_stats, "roi", run_id);

  extract_cache(stats.sim_cache_stats, "sim", run_id);
  extract_cache(stats.roi_cache_stats, "roi", run_id);

  extract_dram(stats.sim_dram_stats, "sim", run_id);
  extract_dram(stats.roi_dram_stats, "roi", run_id);
}

void phase_metric_extractor::emit(std::string_view run_id, std::string_view phase, std::string_view scope, std::string_view metric_id, metric_value value,
                                  std::map<std::string, std::string> dimensions) const
{
  // 输出之前先确认该指标已经在注册表中定义。
  // 如果忘记注册，就说明代码有配置错误，应尽早抛异常。
  if (!registry_.contains(metric_id)) {
    throw std::logic_error{"Metric is not registered: " + std::string{metric_id}};
  }

  sink_.write(metric_record{"1.0", std::string{run_id}, std::string{phase}, std::string{scope}, std::string{metric_id}, std::move(value), registry_.get(metric_id).unit,
                            std::move(dimensions)});
}

void phase_metric_extractor::extract_cpu(const std::vector<O3_CPU::stats_type>& cpu_stats, const std::string& phase, std::string_view run_id) const
{
  for (std::size_t cpu = 0; cpu < cpu_stats.size(); ++cpu) {
    const auto& stats = cpu_stats.at(cpu);
    const auto scope = "cpu" + std::to_string(cpu);

    emit(run_id, phase, scope, "instructions", as_count(static_cast<std::uint64_t>(stats.instrs())));
    emit(run_id, phase, scope, "cycles", as_count(static_cast<std::uint64_t>(stats.cycles())));

    if (stats.cycles() > 0) {
      emit(run_id, phase, scope, "ipc", static_cast<double>(stats.instrs()) / static_cast<double>(stats.cycles()));
    }
  }
}

void phase_metric_extractor::extract_cache(const std::vector<CACHE::stats_type>& cache_stats, const std::string& phase, std::string_view run_id) const
{
  constexpr std::array access_types{access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION};

  for (const auto& stats : cache_stats) {
    for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
      for (const auto type : access_types) {
        const auto dimensions = std::map<std::string, std::string>{{"access_type", access_type_name(type)}};
        emit(run_id, phase, stats.name, "cache_hit", as_count(static_cast<std::uint64_t>(stats.hits.value_or({type, cpu}, 0))), dimensions);
        emit(run_id, phase, stats.name, "cache_miss", as_count(static_cast<std::uint64_t>(stats.misses.value_or({type, cpu}, 0))), dimensions);
        emit(run_id, phase, stats.name, "cache_miss_merge", as_count(static_cast<std::uint64_t>(stats.miss_merge.value_or({type, cpu}, 0))), dimensions);
        emit(run_id, phase, stats.name, "cache_fill", as_count(static_cast<std::uint64_t>(stats.fill.value_or({type, cpu}, 0))), dimensions);
      }
    }

    emit(run_id, phase, stats.name, "prefetch_requested", as_count(stats.pf_requested));
    emit(run_id, phase, stats.name, "prefetch_issued", as_count(stats.pf_issued));
    emit(run_id, phase, stats.name, "prefetch_useful", as_count(stats.pf_useful));
    emit(run_id, phase, stats.name, "prefetch_useless", as_count(stats.pf_useless));
    emit(run_id, phase, stats.name, "prefetch_fill", as_count(stats.pf_fill));

    const auto source_count = std::min<std::size_t>(stats.prefetch_source_count, champsim::prefetching::max_prefetch_sources);
    for (std::size_t source = 0; source < source_count; ++source) {
      const auto dimensions = std::map<std::string, std::string>{{"prefetcher_id", std::to_string(source)}};
      const auto issued = stats.pf_source_issued.at(source);
      const auto useful = stats.pf_source_useful.at(source);
      const auto timely = stats.pf_source_timely.at(source);
      const auto late = stats.pf_source_late.at(source);
      const auto expired = stats.pf_source_expired.at(source);
      const auto timing_fills = stats.pf_source_timing_fills.at(source);

      emit(run_id, phase, stats.name, "prefetch_source_issued", as_count(issued), dimensions);
      emit(run_id, phase, stats.name, "prefetch_source_useful", as_count(useful), dimensions);
      emit(run_id, phase, stats.name, "prefetch_source_useless", as_count(stats.pf_source_useless.at(source)), dimensions);
      emit(run_id, phase, stats.name, "prefetch_source_timely", as_count(timely), dimensions);
      emit(run_id, phase, stats.name, "prefetch_source_late", as_count(late), dimensions);
      emit(run_id, phase, stats.name, "prefetch_source_expired", as_count(expired), dimensions);
      emit(run_id, phase, stats.name, "prefetch_source_timing_fills", as_count(timing_fills), dimensions);

      if (issued > 0) {
        emit(run_id, phase, stats.name, "prefetch_source_accuracy", static_cast<double>(useful) / static_cast<double>(issued), dimensions);
      }
      if (timely + late > 0) {
        emit(run_id, phase, stats.name, "prefetch_source_timely_rate", static_cast<double>(timely) / static_cast<double>(timely + late), dimensions);
      }
      if (timing_fills > 0) {
        emit(run_id, phase, stats.name, "prefetch_source_expiration_rate", static_cast<double>(expired) / static_cast<double>(timing_fills),
             dimensions);
      }
    }

    // 预取提前量原始值。
    emit(run_id, phase, stats.name, "prefetch_timing_fills", as_count(stats.pf_timing_fills));
    emit(run_id, phase, stats.name, "prefetch_timing_timely", as_count(stats.pf_timing_timely));
    emit(run_id, phase, stats.name, "prefetch_timing_late", as_count(stats.pf_timing_late));
    emit(run_id, phase, stats.name, "prefetch_timing_expired", as_count(stats.pf_timing_expired));
    emit(run_id, phase, stats.name, "prefetch_lead_time_sum_cycles", static_cast<std::int64_t>(stats.pf_lead_time_sum_cycles));

    const auto measured_prefetches = stats.pf_timing_timely + stats.pf_timing_late;
    if (measured_prefetches > 0) {
      emit(run_id, phase, stats.name, "prefetch_mean_lead_time",
           static_cast<double>(stats.pf_lead_time_sum_cycles) / static_cast<double>(measured_prefetches));
      emit(run_id, phase, stats.name, "prefetch_timely_rate", static_cast<double>(stats.pf_timing_timely) / static_cast<double>(measured_prefetches));
      emit(run_id, phase, stats.name, "prefetch_late_rate", static_cast<double>(stats.pf_timing_late) / static_cast<double>(measured_prefetches));
    }

    if (stats.pf_timing_fills > 0) {
      emit(run_id, phase, stats.name, "prefetch_expiration_rate",
           static_cast<double>(stats.pf_timing_expired) / static_cast<double>(stats.pf_timing_fills));
    }

    if (stats.pf_issued > 0) {
      emit(run_id, phase, stats.name, "prefetch_accuracy", static_cast<double>(stats.pf_useful) / static_cast<double>(stats.pf_issued));
    }

    std::uint64_t demand_fills = static_cast<std::uint64_t>(stats.fill.total());
    for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu) {
      const auto prefetch_fills = static_cast<std::uint64_t>(stats.fill.value_or({access_type::PREFETCH, cpu}, 0));
      demand_fills -= std::min(demand_fills, prefetch_fills);
    }

    if (demand_fills > 0) {
      emit(run_id, phase, stats.name, "prefetch_coverage", static_cast<double>(stats.pf_useful) / static_cast<double>(demand_fills));
      emit(run_id, phase, stats.name, "cache_average_miss_latency",
           static_cast<double>(stats.total_miss_latency_cycles) / static_cast<double>(demand_fills));
    }
  }
}

void phase_metric_extractor::extract_dram(const std::vector<DRAM_CHANNEL::stats_type>& dram_stats, const std::string& phase, std::string_view run_id) const
{
  for (const auto& stats : dram_stats) {
    emit(run_id, phase, stats.name, "dram_rq_row_buffer_hit", as_count(stats.RQ_ROW_BUFFER_HIT));
    emit(run_id, phase, stats.name, "dram_rq_row_buffer_miss", as_count(stats.RQ_ROW_BUFFER_MISS));
    emit(run_id, phase, stats.name, "dram_wq_row_buffer_hit", as_count(stats.WQ_ROW_BUFFER_HIT));
    emit(run_id, phase, stats.name, "dram_wq_row_buffer_miss", as_count(stats.WQ_ROW_BUFFER_MISS));
    emit(run_id, phase, stats.name, "dram_wq_full", as_count(stats.WQ_FULL));
    emit(run_id, phase, stats.name, "dram_refreshes_issued", as_count(stats.refresh_cycles));

    if (stats.dbus_count_congested > 0) {
      emit(run_id, phase, stats.name, "dram_avg_dbus_congested_cycle",
           static_cast<double>(stats.dbus_cycle_congested) / static_cast<double>(stats.dbus_count_congested));
    }
  }
}
} // namespace champsim::metrics

