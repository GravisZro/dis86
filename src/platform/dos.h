#pragma once
#include <cstdint>
#include <unistd.h>

namespace dos
{
  using paragraph_t = uint8_t[16];
  using block_t = uint8_t[512];

  struct [[gnu::packed]] executable_header_t
  {
    uint16_t signature; // 0x5a4D
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
  static_assert(sizeof(executable_header_t) == 28);

  struct [[gnu::packed]] relocation_t
  {
    uint16_t offset; // Offset of the relocation within provided segment.
    uint16_t segment; // Segment of the relocation, relative to the load segment address.
  };


  struct executable_layout_t
  {
    constexpr size_t exe_offset(void) { return header.header_paragraphs * sizeof(paragraph_t); }

    constexpr size_t extra_offset(void)
    {
      return header.blocks_in_file * sizeof(block_t) -
             (header.bytes_in_last_block ? 512 - header.bytes_in_last_block : 0);
    }

    executable_header_t header;
  };
}

