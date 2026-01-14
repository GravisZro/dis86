#include "region.h"

#include <bit>
#include <cassert>

namespace region
{
  using namespace segoff;

  RegionIter_t::RegionIter_t(segment<uint8_t> seg, const SegOff_t& segoff)
      : mem(seg),
        base_seg(segoff.seg),
        base_off(segoff.off),
        off(0)
  { }

  void RegionIter_t::reset_addr(const SegOff_t& addr)
  {
    assert(base_addr() <= addr && addr < end_addr());
    off = addr.off - base_off;
  }

  Result<uint8_t, std::string> RegionIter_t::get_checked(SegOff_t o_addr) const
  {
    if (o_addr.seg != base_seg)
      return std::string("Mismatching segments");
    if(o_addr.off < base_off)
      return std::string("RegionIter access below start of region");
    if(o_addr.off >= base_off + mem.size())
      return std::string("RegionIter access beyond end of region");
    return mem[o_addr.off - base_off];
  }

  segment<uint8_t> RegionIter_t::slice(SegOff_t addr, uint16_t len) const
  {
    if(addr.seg != base_seg)
      assert("Mismatching segments" && false);
    if(addr.off < base_off)
      assert("RegionIter access below start of region" && false);
    if(addr.off + len > base_off + mem.size())
      assert("RegionIter access beyond end of region" && false);
    return segment<uint8_t>(mem.data() + addr.off - base_off, len);
  }

  Result<uint8_t, std::string> RegionIter_t::fetch(void)
  {
    auto v = peek_checked();
    advance();
    return v;
  }

  Result<uint16_t, std::string> RegionIter_t::fetch_sext(void)
  {
    auto v = fetch();
    if(v.is_err())
      return v.error();
    return std::bit_cast<uint16_t>(int16_t(*reinterpret_cast<const int8_t*>(&v.value())));
  }

  Result<uint16_t, std::string> RegionIter_t::fetch_u16(void)
  {
    uint8_t upper = 0, lower = 0;

    auto v = fetch();
    if(v.is_err())
      return v.error();
    lower = v.value();

    v = fetch();
    if(v.is_err())
      return v.error();
    upper = v.value();

    return uint16_t((uint16_t(upper) << 8) | lower);
  }
}


#ifdef ENABLE_TESTS
#include <cassert>
#include <iostream>
namespace region
{
  void test(void)
  {
    uint8_t data[] = { 0x12, 0x34, 0x56, 0x78, 0x9a };
    segment<uint8_t> data_seg = { data, sizeof(data) };
    auto result = SegOff_t::from_str("0000:000a");
    assert(result.is_ok());
    SegOff_t& addr = result.value();

    RegionIter_t b(data_seg, addr);

    assert(b.peek() == 0x12);
    assert(b.peek() == 0x12);

    b.advance();
    assert(b.peek() == 0x34);
    assert(b.get(addr) == 0x12);

    {
      auto v = b.fetch();
      if(v.is_err())
        std::cout << v.error() << std::endl;
      assert(v.value() == 0x34);
    }
    assert(b.peek() == 0x56);

    {
      auto v = b.fetch_u16();
      if(v.is_err())
        std::cout << v.error() << std::endl;
      assert(v.value() == 0x7856);
    }

    assert(b.peek() == 0x9a);
    std::cout << "region test passed" << std::endl;
  }
}
#endif
