#pragma once
#include <string>
#include <cstdint>

#include "segoff.h"
#include "common/segment.h"
#include "common/result.h"


namespace region
{
  using namespace segoff;
  struct RegionIter_t
  {
    segment<uint8_t> mem;
    Seg_t       base_seg;
    Off_t       base_off;
    std::size_t off;

    RegionIter_t(segment<uint8_t> seg, const SegOff_t& segoff);

    void reset_addr(const SegOff_t& addr);
    SegOff_t base_addr(void) const { return { base_seg, base_off }; }
    SegOff_t end_addr(void) const { return base_addr().add_offset(mem.size()); }
    SegOff_t addr(void) const { return base_addr().add_offset(off); }
    std::size_t bytes_remaining(void) const { return mem.size() - off; }

    Result<uint8_t, std::string> get_checked(SegOff_t o_addr) const;
    Result<uint8_t, std::string> peek_checked(void) const
    { return get_checked(addr()); }

    uint8_t get(SegOff_t segoff) const { return mem[segoff.off - base_off]; }
    uint8_t peek(void) const { return get(addr()); }

    void advance(void) { off++; }
    void advance_by(std::size_t n) { off += n; }

    segment<uint8_t> slice(SegOff_t addr, uint16_t len) const;
    Result<uint8_t, std::string> fetch(void);
    Result<uint16_t, std::string> fetch_sext(void);
    Result<uint16_t, std::string> fetch_u16(void);
  };
}
