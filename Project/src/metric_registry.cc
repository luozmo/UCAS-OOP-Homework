# include "metric_registry.h"

# include <stdexcept>
# include <string>
# include <utility>

namespace champsim::metrics
{
void metric_registry::add(metric_definition definition)
{
  if (definition.metric_id.empty()) {
    throw std::invalid_argument{"Metric definition must have a non-empty metric_id."};
  }

  if (definition.kind == metric_kind::derived && !definition.formula.has_value()) {
    throw std::invalid_argument{"Derived metric must define a formula."};
  }

  if (definition.kind != metric_kind::derived && definition.formula.has_value()) {
    throw std::invalid_argument{"Only derived metrics may define a formula."};
  }

  const auto metric_id = definition.metric_id;
  if (definitions_.find(metric_id) != std::end(definitions_)) {
    throw std::invalid_argument{"Metric definition already exists: " + metric_id};
  }

  definitions_.emplace(metric_id, std::move(definition));
}

bool metric_registry::contains(std::string_view metric_id) const { return definitions_.find(metric_id) != std::end(definitions_); }

std::optional<metric_definition> metric_registry::find(std::string_view metric_id) const
{
  const auto iter = definitions_.find(metric_id);
  if (iter == std::end(definitions_)) {
    return std::nullopt;
  }
  return iter->second;
}

const metric_definition& metric_registry::get(std::string_view metric_id) const
{
  const auto iter = definitions_.find(metric_id);
  if (iter == std::end(definitions_)) {
    throw std::out_of_range{"Unknown metric definition: " + std::string{metric_id}};
  }
  return iter->second;
}

std::size_t metric_registry::size() const { return std::size(definitions_); }

const std::map<std::string, metric_definition, std::less<>>& metric_registry::definitions() const { return definitions_; }
} // namespace champsim::metrics
