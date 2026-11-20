# ifndef METRIC_RECORD_H
# define METRIC_RECORD_H

# include <cstdint>
# include <map>
# include <string>
# include <variant>

namespace champsim::metrics
{

using metric_value = std::variant<std::int64_t, double>;

struct metric_record {
  std::string schema_version{"1.0"};
  std::string run_id;
  std::string phase;
  std::string scope;
  std::string metric_id;
  metric_value value{std::int64_t{0}};
  std::string unit;
  std::map<std::string, std::string> dimensions{};
};

} // namespace champsim::metrics

# endif
