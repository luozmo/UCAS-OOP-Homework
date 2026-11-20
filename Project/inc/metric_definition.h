# ifndef METRIC_DEFINITION_H
# define METRIC_DEFINITION_H

# include <optional>
# include <string>
# include <string_view>
# include <vector>

namespace champsim::metrics
{

enum class metric_kind {
  raw,
  derived,
  instrumented,
};

constexpr std::string_view to_string(metric_kind kind)
{
  switch (kind) {
  case metric_kind::raw:
    return "raw";
  case metric_kind::derived:
    return "derived";
  case metric_kind::instrumented:
    return "instrumented";
  }
  return "unknown";
}

struct metric_definition {
  std::string metric_id;
  std::string unit;
  metric_kind kind{metric_kind::raw};
  std::optional<std::string> formula{};
  std::vector<std::string> dimensions{};
  std::string description{};
};

} // namespace champsim::metrics

# endif
