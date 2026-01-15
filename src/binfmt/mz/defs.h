#pragma once
#include <cstdint>

#include "segoff.h"

namespace binfmt::mz
{
  using namespace segoff;

  using paragraph_t = uint8_t[16];
  using block_t = uint8_t[512];

  struct  [[gnu::packed]] Header
  {
    char     signature[2]; // 0x5A4D == "MZ"
    uint16_t bytes_in_last_block;
    uint16_t blocks_in_file;
    uint16_t num_relocs;
    uint16_t header_paragraphs;
    uint16_t min_extra_paragraphs;
    uint16_t max_extra_paragraphs;
    uint16_t ss;
    uint16_t sp;
    uint16_t checksum;
    uint16_t ip;
    uint16_t cs;
    uint16_t reloc_table_offset;
    uint16_t overlay_number;
  };
  static_assert(sizeof(Header) == 28);

  struct [[gnu::packed]] Reloc
  {
    uint16_t offset; // Offset of the relocation within provided segment.
    uint16_t segment; // Segment of the relocation, relative to the load segment address.
  };

  // Borland C/C++ FBOV Header for Overlays (VROOM?)
  struct [[gnu::packed]] FBOV
  {
    char     signature[4]; /* "FBOV" */
    uint32_t ovrsize;
    uint32_t exeinfo;  /* points to mz_seginfo array in binary */
    int32_t  segnum;   /* number of entries in the mz_seginfo array */
  };
  static_assert(sizeof(FBOV) == 16);

  enum class SegInfoType : uint16_t
  {
    DATA = 0,
    CODE,
    STUB,
    OVERLAY,
  };

  struct [[gnu::packed]] SegInfo
  {
    uint16_t    seg;
    uint16_t    maxoff;
    SegInfoType typ;
    uint16_t    minoff;

    constexpr uint16_t size(void) const
      { return maxoff - minoff; }
  };
  static_assert(sizeof(SegInfo) == 8);

  struct [[gnu::packed]] OverlaySeg
  {
    uint16_t stub_segment;     // Segment number where the stubs are located
    uint16_t segment_size;     // Size of the destination segment
    uint32_t data_offset;      // Offset to the destination segment in the binary image (from OverlayInfo::file_offset)
    uint16_t _unknown_1;
    uint16_t _unknown_2;
  };
  static_assert(sizeof(OverlaySeg) == 12);

  struct [[gnu::packed]] OverlayStub
  {
    uint16_t overlay_seg_num;  // Id or index of the overlay segment this stub belongs to
    uint16_t stub_segment;     // Segment this stub is located at (as called)
    uint16_t stub_offset;      // Offset this stub is located at (as called)
    uint16_t dest_offset;      // Destination offset into the overlay segment (wherever it ends up resident)

    SegOff_t stub_addr(void) const
      { return { Seg_t { Seg_t::Normal, stub_segment }, stub_offset }; }

    SegOff_t dest_addr(void) const
      { return SegOff_t { Seg_t { Seg_t::Overlay, overlay_seg_num }, dest_offset }; }
  };
  static_assert(sizeof(OverlayStub) == 8);
}
