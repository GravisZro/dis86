#pragma once
#include "segoff.h"
#include "config.h"

#include <optional>
#include <string>

namespace spec
{
  using namespace segoff;
  using namespace config;

  struct Spec
  {
    std::optional<Func> func;
    std::string name;
    SegOff_t start;
    SegOff_t end;

    static Spec from_config_name(Config& cfg, std::string name);
    static Spec from_start_and_end(std::optional<SegOff_t> start, std::optional<SegOff_t> end);
  };
}
