#include "overlay.h"

#include <format>
#include <array>

namespace overlay
{
  using namespace binfmt::mz;

  static constexpr const std::array<uint8_t,  4> segment_interrupt_code = { 0xcd, 0x3f, 0x00, 0x00 };
  static constexpr const std::array<uint8_t, 18> segment_zeroes         = { 0x00 };
  static constexpr const std::array<uint8_t,  2> stub_interrupt_code    = { 0xcd, 0x3f };
  static constexpr const std::array<uint8_t,  1> stub_zeroes            = { 0x00 };

  template<typename Arr>
  std::string print_array(const Arr& data)
  {
    static constexpr const std::array<uint8_t, 16> hex_digits =
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    std::string rval;
    rval.reserve(data.size() * 3);
    for(uint8_t val : data)
    {
      rval.push_back(hex_digits[val >> 4]);
      rval.push_back(hex_digits[val & 0x0F]);
      rval.push_back(' ');
    }
    rval.pop_back();
    return rval;
  }

  struct [[gnu::packed]] CodeOverlaySeg
  {
    std::array<uint8_t, 4> interrupt_code; // should be: cd 3f 00 00
    uint32_t data_offset;
    uint16_t seg_size;
    uint16_t _unknown_1;
    uint16_t _unknown_2;
    std::array<uint8_t, 18> _zeros;
    /* stubs follow: [CodeOverlaySeg] */
  };
  static_assert(sizeof(CodeOverlaySeg) == 32);

  struct [[gnu::packed]] CodeOverlayStub
  {
    std::array<uint8_t, 2> interrupt_code; // should be: cd 3f
    uint16_t call_offset;
    std::array<uint8_t, 1> _zeros;
  };
  static_assert(sizeof(CodeOverlayStub) == 5);


  Result<OverlayInfo, std::string> decode_overlay_info(segment<uint8_t> data, uint32_t exe_start, FBOV* fbov, segment<SegInfo> seginfo)
  {
    OverlayInfo rval;

    // Overlay data starts immediately after the FBOV header
    rval.file_offset = reinterpret_cast<uint8_t*>(fbov + 1) - data.data();

    uint32_t* exe_data = data.ptr_at<uint32_t>(exe_start);
    // let exe_data = exe.exe_data();
    // let Some(seginfo) = exe.seginfo else {
    //   return Ok(None);
    // };

    auto next_seg = 0;
    for(std::size_t segnum = 0; segnum < seginfo.size(); ++segnum)
    {
      auto& s = seginfo[segnum];
      // iterate all stubs
      if(s.typ != SegInfoType::STUB)
        continue;

      // sanity check: might not be required but it's generally true how with
      // how this compiler liked to layout things
      assert(next_seg == 0 || s.seg == next_seg);
      auto n_segs = (s.maxoff + 15) >> 4;
      next_seg = s.seg + n_segs;

      // unpack the actual stub code
      uint32_t* dat = exe_data + 16 * s.seg;
      std::size_t sz = s.maxoff;
      assert(sz >= 32); // each hdr section is 32-bytes
      assert((sz - 32) % 5 == 0); // each launcher entry is 5 bytes
      std::size_t num_entries = (sz - 32) / 5;

      // get the seg struct
      CodeOverlaySeg* seg = reinterpret_cast<CodeOverlaySeg*>(dat);
      if(seg->interrupt_code != segment_interrupt_code)
        return std::format("Invalid seg interrupt code, got {} expected {}", 
            print_array(seg->interrupt_code),
            print_array(segment_interrupt_code));

      if(seg->_zeros != segment_zeroes)
        return std::format("Zeros in seg aren't zero, got {} expected {}", 
            print_array(seg->_zeros),
            print_array(segment_zeroes));

      // create a user struct
      rval.segs.emplace_back(
          OverlaySeg { s.seg,
                     seg->seg_size,
                     seg->data_offset,
                     seg->_unknown_1,
                     seg->_unknown_2 });

      uint16_t seg_num = rval.segs.size() - 1;

      // process each stub
      segment<CodeOverlayStub> stubs = { seg + 1, num_entries };
      for(std::size_t i = 0; i < stubs.size(); ++i)
      {
        CodeOverlayStub& stub = stubs[i];

        if(stub.interrupt_code != stub_interrupt_code)
          return std::format("Invalid stub interrupt code, got {} expected {}", 
              print_array(stub.interrupt_code),
              print_array(stub_interrupt_code));

        if(stub._zeros != stub_zeroes)
          return std::format("Zeros in stub aren't zero, got {} expected {}", 
              print_array(stub._zeros),
              print_array(stub_zeroes));

        if(stub.call_offset >= seg->seg_size)
        {
          return std::format("Stub call offset exceeds the segment size, offset {} segsize: {}", 
              uint16_t(stub.call_offset),
              uint16_t(seg->seg_size));
        }

        rval.stubs.emplace_back(
            OverlayStub {
                seg_num,
                s.seg,
                uint16_t(sizeof(CodeOverlaySeg) + 5 * i),
                stub.call_offset,
            });
      }
    }

    return rval;
  }
}
