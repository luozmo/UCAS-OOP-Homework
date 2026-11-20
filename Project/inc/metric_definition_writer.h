#ifndef METRIC_DEFINITION_WRITER_H
#define METRIC_DEFINITION_WRITER_H

#include <ostream>

#include "metric_registry.h"

namespace champsim::metrics
{

// metric_definition_writer 专门负责把“指标说明书”写成 JSON 文件。
//
// 它不写具体某次运行的值，只写：
//   ipc 是什么、单位是什么、公式是什么、有哪些维度。
//
// 真正的指标值由 metric_sink / jsonl_metric_sink 负责输出。
class metric_definition_writer
{
public:
  // 创建 writer 时必须告诉它输出到哪里。
  explicit metric_definition_writer(std::ostream& stream);

  // 从注册表读取全部 MetricDefinition，并写成 JSON。
  void write(const metric_registry& registry) const;

private:
  std::ostream& stream_;
};

} // namespace champsim::metrics

#endif
