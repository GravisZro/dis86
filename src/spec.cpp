#include "spec.h"

#include "common/print.h"

namespace spec
{
  using namespace segoff;
  using namespace config;

  Spec Spec::from_config_name(Config& cfg, std::string name)
  {
    Spec rval;
    rval.name = name;
    rval.func = cfg.func_lookup_by_name(name);
    if(!rval.func)
      panic("Failed to lookup function named: {}", name);

    SegOff_t start, end;
    if(rval.func->overlay)
    {
      rval.start = { Seg_t { Seg_t::Overlay, rval.func->overlay->num }, rval.func->overlay->start };
      rval.end   = { Seg_t { Seg_t::Overlay, rval.func->overlay->num }, rval.func->overlay->end   };
    }
    else
    {
      rval.start = rval.func->start;
      if(!rval.func->end)
        panic("Function has no 'end' addr defined in config");

      rval.end = *rval.func->end;
    }
    return rval;
  }

  Spec Spec::from_start_and_end(std::optional<SegOff_t> start, std::optional<SegOff_t> end)
  {
    if(start)
      panic("No start address provided");
    if(end)
      panic("No end address provided");

    return Spec { {},
                std::format("func_{}_{}", static_cast<uint16_t>(start->seg), start->off),
                *start,
                *end };
  }
}
