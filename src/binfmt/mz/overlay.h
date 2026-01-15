#pragma once

#include <cstdint>

#include "common/segment.h"
#include "common/result.h"

#include "defs.h"

namespace overlay
{
  using namespace binfmt::mz;

  struct OverlayInfo
  {
    uint32_t file_offset;
    std::vector<OverlaySeg> segs;
    std::vector<OverlayStub> stubs;
  };

  Result<OverlayInfo, std::string> decode_overlay_info(
      segment<uint8_t> data,
      uint32_t exe_start,
      FBOV* fbov,
      segment<SegInfo> seginfo);
}
