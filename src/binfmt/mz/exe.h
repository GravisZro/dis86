#pragma once
#include <optional>

#include "common/segment.h"
#include "defs.h"
#include "overlay.h"

namespace binfmt::mz
{
  struct Exe
  {
    Header* hdr = nullptr;
    uint32_t exe_start = 0;
    uint32_t exe_end = 0;
    segment<Reloc> relocs;
    FBOV* fbov = nullptr;
    segment<SegInfo> seginfo;
    std::optional<overlay::OverlayInfo> ovr;
    segment<uint8_t> rawdata;

    segment<uint8_t> exe_data(void) const;
    segment<uint8_t> overlay_data(std::size_t id) const;
    std::size_t num_overlay_segments(void) const;

    static Result<Exe, std::string> decode(segment<uint8_t> data);

    void print_hdr(void) const;
    void print_relocs(segment<Reloc> relocs) const;
    void print_fbov(FBOV* fbov) const;
    void print_seginfo(segment<SegInfo> seginfo) const;
    void print_overlayinfo(const overlay::OverlayInfo& ovr) const;
    void print_exe(void) const;
  };
}
