#include <string>

#include "rapidutf/rapidutf.hpp"

#include <fmt/core.h>

exported_class::exported_class()
    : m_name {fmt::format("{}", "rapidutf")}
{
}

auto exported_class::name() const -> const char*
{
  return m_name.c_str();
}
