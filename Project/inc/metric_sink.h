#ifndef METRIC_SINK_H
#define METRIC_SINK_H

#include "metric_record.h"

namespace champsim::metrics
{

// metric_sink 是一个“接口类”。
// 它只规定“指标应该怎样被写出去”，不关心到底写到文件、屏幕还是内存字符串。
//
// 例如：
//   - jsonl_metric_sink 会写成 JSONL
//   - 以后还可以增加 csv_metric_sink、json_metric_sink 等
class metric_sink
{
public:
  // 基类析构函数必须是 virtual。
  // 这样通过 metric_sink* 删除子类对象时，子类析构函数也能正确执行。
  virtual ~metric_sink() = default;

  // “= 0”表示这是纯虚函数：
  // metric_sink 自己不知道怎么写，必须由具体子类实现。
  virtual void write(const metric_record& record) = 0;

  // 有些输出方式需要显式刷新缓冲区。
  // 默认什么都不做，具体子类可以覆盖它。
  virtual void flush() {}
};

} // namespace champsim::metrics

#endif
