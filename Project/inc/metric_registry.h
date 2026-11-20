# ifndef METRIC_REGISTRY_H
# define METRIC_REGISTRY_H

# include <map>
# include <optional>
# include <string_view>

# include "metric_definition.h"

namespace champsim::metrics
{

class metric_registry
{
public:
  void add(metric_definition definition);

  [[nodiscard]] bool contains(std::string_view metric_id) const;
  [[nodiscard]] std::optional<metric_definition> find(std::string_view metric_id) const;
  [[nodiscard]] const metric_definition& get(std::string_view metric_id) const;
  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] const std::map<std::string, metric_definition, std::less<>>& definitions() const;

private:
  std::map<std::string, metric_definition, std::less<>> definitions_{};
};

} // namespace champsim::metrics

# endif
