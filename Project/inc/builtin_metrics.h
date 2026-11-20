#ifndef BUILTIN_METRICS_H
#define BUILTIN_METRICS_H

#include "metric_registry.h"

namespace champsim::metrics
{

// 创建当前 ChampSim 统计对应的内置指标定义。
//
// 这些定义只说明指标名称、单位和公式，不包含某次运行的具体值。
metric_registry make_builtin_metric_registry();

} // namespace champsim::metrics

#endif
