#pragma once

#include <cstdint>
#include <cassert>
#include <unistd.h>
#include <variant>
#include <string>
#include <format>
#include <regex>

#define panic(...)

using Off_t = uint16_t;
using Seg_t = uint16_t;

struct SegOff_t
{
  enum type_e
  {
    Normal,
    Overlay,
  };

  SegOff_t(SegOff_t&&) = default;
  SegOff_t(SegOff_t::type_e t, Seg_t s, Off_t o)
      : type(t), seg(s), off(o) { }
  SegOff_t(void) : SegOff_t(Overlay, 0, 0) { }
  //constexpr SegOff_t& operator =(SegOff_t&&) = default;

  type_e type;
  Seg_t seg;
  Off_t off;


  size_t abs_normal(void) const
  {
    assert(type == Normal);
    return size_t(seg) * 16 + off;
  }

  bool is_overlay_addr(void) const { return type == Overlay; }

  SegOff_t add_offset(uint16_t offset) const
    { return { type, seg, uint16_t(off + offset) }; }


  uint16_t offset_to(const SegOff_t& other)
  {
    if(seg != other.seg) { panic("Cannot take difference of different segments"); }
    if(off > other.off) { panic("Not a positive offset"); }
    return other.off - off;
  }

  std::variant<SegOff_t, std::string> from_str(std::string s)
  {
    const std::regex segoff_regex ("^([[:xdigit:]]{4}):([[:xdigit:]]{4})$");
    const std::regex overlay_regex("^([[:xdigit:]]{2}):([[:xdigit:]]{4})$");
    std::smatch match;
    if (std::regex_match(s, match, segoff_regex) && match.size() == 3)
      return SegOff_t { Overlay, uint16_t(std::stoi(match[1].str())), uint16_t(std::stoi(match[2].str())) };
    if (std::regex_match(s, match, overlay_regex) && match.size() == 3)
      return SegOff_t { Normal, uint16_t(std::stoi(match[1].str())), uint16_t(std::stoi(match[2].str())) };
    return std::format<"Invalid segoff: '{}'">(s);
  }

  operator std::string(void) const
  {
    if(type == Overlay)
      return std::format<"%04x:%04x">(seg, off);
    else
      return std::format<"%02x:%04x">(seg, off);
  }
};
