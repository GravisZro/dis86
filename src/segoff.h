#pragma once

#include <cstdint>
#include <string>

#include "common/result.h"

namespace segoff
{
  class Seg_t
  {
  public:
    enum type_e : uint8_t
    {
      Normal,
      Overlay,
    };

    Seg_t(type_e t, uint16_t v) : type(t), value(v) { }
    Seg_t(const Seg_t&) = default;

    constexpr bool operator ==(const Seg_t& o) const
      { return o.type == type && o.value == value; }

    explicit constexpr operator type_e(void) const { return type; }
    constexpr operator uint16_t (void) const { return value; }

    uint16_t unwrap_normal(void) const;
    uint16_t unwrap_overlay(void) const;

  private:
    type_e type;
    uint16_t value;
  };

  using Off_t = uint16_t;

  struct SegOff_t
  {
    SegOff_t(Seg_t s, Off_t o) : seg(s), off(o) { }
    SegOff_t(void) : seg(Seg_t::Normal, 0), off(0) { }

    Seg_t seg;
    Off_t off;

    std::size_t abs_normal(void) const
      { return std::size_t(seg.unwrap_normal()) + off; }

    bool is_overlay_addr(void) const { return seg == Seg_t::Overlay; }

    SegOff_t add_offset(uint16_t offset) const
      { return SegOff_t { seg, Off_t(off + offset) }; }

    uint16_t offset_to(const SegOff_t& other);

    static Result<SegOff_t, std::string> from_str(std::string s);

    std::string to_str(void) const;

    bool operator > (const SegOff_t& o) const { return seg > o.seg || (seg == o.seg && off > o.off); }
    bool operator < (const SegOff_t& o) const { return seg < o.seg || (seg == o.seg && off < o.off); }
    bool operator ==(const SegOff_t& o) const { return seg == o.seg && off == o.off; }
    bool operator >=(const SegOff_t& o) const { return operator >(o) || operator ==(o); }
    bool operator <=(const SegOff_t& o) const { return operator <(o) || operator ==(o); }
  };
}
