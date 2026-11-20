#include "metric_definition_writer.h"

#include <nlohmann/json.hpp>

namespace champsim::metrics
{
metric_definition_writer::metric_definition_writer(std::ostream& stream) : stream_(stream) {}

void metric_definition_writer::write(const metric_registry& registry) const
{
  // JSON 的顶层结构：
  // {
  //   "schema_version": "1.0",
  //   "metrics": { ... }
  // }
  nlohmann::json metrics = nlohmann::json::object();

  // map 的每个元素都是：
  //   metric_id -> metric_definition
  for (const auto& [metric_id, definition] : registry.definitions()) {
    // formula 是可选项。
    // 如果没有公式，就明确输出 null，而不是把 formula 字段整个省略。
    // 这样所有指标定义的顶层字段保持一致，后续解析更简单。
    nlohmann::json formula = nullptr;
    if (definition.formula.has_value()) {
      formula = *definition.formula;
    }

    nlohmann::json definition_json{
        {"unit", definition.unit},
        {"kind", to_string(definition.kind)},
        {"formula", formula},
        {"dimensions", definition.dimensions},
        {"description", definition.description},
    };

    // 把当前指标定义放进 "metrics" 对象。
    // 最后会形成：
    //   "ipc": { ... },
    //   "load_miss": { ... }
    metrics[metric_id] = definition_json;
  }

  nlohmann::json output{
      {"schema_version", "1.0"},
      {"metrics", metrics},
  };

  // dump(2) 表示缩进 2 个空格，方便人类阅读。
  stream_ << output.dump(2) << '\n';
}
} // namespace champsim::metrics
