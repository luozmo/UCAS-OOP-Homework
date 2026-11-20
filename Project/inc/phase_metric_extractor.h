#ifndef PHASE_METRIC_EXTRACTOR_H
#define PHASE_METRIC_EXTRACTOR_H

#include <cstdint>
#include <map>
#include <string>
#include <string_view>

#include "cache.h"
#include "dram_controller.h"
#include "metric_registry.h"
#include "metric_sink.h"
#include "ooo_cpu.h"
#include "phase_info.h"

namespace champsim::metrics
{

// phase_metric_extractor 负责：
//   ChampSim phase_stats -> 多条 metric_record -> metric_sink
//
// 它不计算仿真，也不修改仿真对象。
// 它只是一个“转换器”：把旧格式统计转换成统一结构化指标。
class phase_metric_extractor
{
public:
  phase_metric_extractor(const metric_registry& registry, metric_sink& sink);

  // run_id 用来标识这一次独立仿真运行。
  void extract(const champsim::phase_stats& stats, std::string_view run_id) const;

private:
  void extract_cpu(const std::vector<O3_CPU::stats_type>& cpu_stats, const std::string& phase, std::string_view run_id) const;
  void extract_cache(const std::vector<CACHE::stats_type>& cache_stats, const std::string& phase, std::string_view run_id) const;
  void extract_dram(const std::vector<DRAM_CHANNEL::stats_type>& dram_stats, const std::string& phase, std::string_view run_id) const;

  void emit(std::string_view run_id, std::string_view phase, std::string_view scope, std::string_view metric_id, metric_value value,
            std::map<std::string, std::string> dimensions = {}) const;

  const metric_registry& registry_;
  metric_sink& sink_;
};

} // namespace champsim::metrics

#endif

