#ifndef JSONL_METRIC_SINK_H
#define JSONL_METRIC_SINK_H

#include <ostream>

#include "metric_sink.h"

namespace champsim::metrics
{

// jsonl_metric_sink 表示“一条记录写一行 JSON”。
//
// JSONL 示例：
//   {"metric_id":"ipc","value":0.75,...}
//   {"metric_id":"load_miss","value":1234,...}
//
// 它继承 metric_sink，表示它满足统一输出接口。
class jsonl_metric_sink final : public metric_sink
{
public:
  // 构造函数接收一个输出流引用。
  // 例如可以传入 std::ofstream，也可以传入 std::ostringstream。
  explicit jsonl_metric_sink(std::ostream& stream);

  // override 表示这里重写基类的纯虚函数。
  void write(const metric_record& record) override;

  // 把输出流缓冲区中的数据真正写出去。
  void flush() override;

private:
  std::ostream& stream_;
};

} // namespace champsim::metrics

#endif
