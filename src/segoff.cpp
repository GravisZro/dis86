#include "segoff.h"

#include <cassert>
#include <format>
#include <regex>

namespace segoff
{
  uint16_t Seg_t::unwrap_normal(void) const
  {
    assert("Expected Seg_t::Normal" && type == Seg_t::Normal);
    return value;
  }

  uint16_t Seg_t::unwrap_overlay(void) const
  {
    assert("Expected Seg_t::Overlay" && type == Seg_t::Overlay);
    return value;
  }

  uint16_t SegOff_t::offset_to(const SegOff_t& other)
  {
    assert("Cannot take difference of different segments" && seg == other.seg);
    assert("Not a positive offset" && off <= other.off);
    return other.off - off;
  }

  Result<SegOff_t, std::string> SegOff_t::from_str(std::string s)
  {
    // format: 'xxxx:yyyy' where xxxx and yyyy are 16-bit hexdecimal vales
    static const std::regex segoff_regex ("^([[:xdigit:]]{4}):([[:xdigit:]]{4})$");
    // format: 'ovrxx:yyyy' where xx is an 8-bit hexdecimal and yyyy is a 16-bit hexdecimal value
    static const std::regex overlay_regex ("^ovr([[:xdigit:]]{2}):([[:xdigit:]]{4})$");
    std::smatch match;
    if (std::regex_match(s, match, segoff_regex) && match.size() == 3)
      return SegOff_t { Seg_t { Seg_t::Normal, uint16_t(std::stoi(match[1].str())) }, uint16_t(std::stoi(match[2].str())) };
    if (std::regex_match(s, match, overlay_regex) && match.size() == 3)
      return SegOff_t { Seg_t { Seg_t::Overlay, uint16_t(std::stoi(match[1].str())) }, uint16_t(std::stoi(match[2].str())) };
    return std::format("Invalid segoff: '{}'", s);
  }

  std::string SegOff_t::to_string(void) const
  {
    if(seg == Seg_t::Normal)
      return std::format("{:04x}:", seg.unwrap_normal()) + std::format("{:04x}", off);
      //return std::format("{:04x}:{:04x}", seg.unwrap_normal(), off);
    else if(seg == Seg_t::Overlay)
      return std::format("ovr{:02x}:", seg.unwrap_overlay()) + std::format("{:04x}", off);
      //return std::format("ovr{:02x}:{:04x}", seg.unwrap_overlay(), off);
    return "Invalid segoff";
  }
}
