#include "jsonl_metric_sink.h"

#include <variant>
#include <nlohmann/json.hpp>

namespace
{
// metric_record::value 是 std::variant<int64_t, double>。
// std::visit 会根据当前实际保存的类型，调用下面这个 lambda。
//
// 如果 value 里是整数，就生成 JSON 整数；
// 如果 value 里是浮点数，就生成 JSON 浮点数。
nlohmann::json metric_value_to_json(const champsim::metrics::metric_value& value)
{
  return std::visit([](const auto& actual_value) -> nlohmann::json { return actual_value; }, value);
}
} // namespace

namespace champsim::metrics
{
jsonl_metric_sink::jsonl_metric_sink(std::ostream& stream) : stream_(stream) {}

void jsonl_metric_sink::write(const metric_record& record)
{
  // 先把一条记录转换成一个 JSON 对象。
  // 这里字段名和 metric_record 的顶层字段一一对应。
  nlohmann::json output{
      {"schema_version", record.schema_version},
      {"run_id", record.run_id},
      {"phase", record.phase},
      {"scope", record.scope},
      {"metric_id", record.metric_id},
      {"value", metric_value_to_json(record.value)},
      {"unit", record.unit},
      {"dimensions", record.dimensions},
  };

  // dump() 把 JSON 对象变成字符串。
  // '\n' 表示每条记录占一行，这正是 JSONL 的格式。
  stream_ << output.dump() << '\n';
}

void jsonl_metric_sink::flush()
{
  // 例如 std::ofstream 可能先把数据放在缓冲区里。
  // flush() 会要求它立即写入磁盘。
  stream_.flush();
}
} // namespace champsim::metrics
