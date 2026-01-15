#include "exe.h"

#include <format>

namespace binfmt::mz
{
  segment<uint8_t> Exe::exe_data(void) const
  {
    return segment<uint8_t> { rawdata.ptr_at(exe_start),
                            exe_end - exe_start };
  }

  segment<uint8_t> Exe::overlay_data(size_t id) const
  {
    assert(ovr);
    const OverlaySeg& seg = ovr->segs[id];
    size_t start = ovr->file_offset + seg.data_offset;
    size_t end = start + seg.segment_size;
    return segment<uint8_t> { rawdata.ptr_at(start),
                            end - start };
  }


  size_t Exe::num_overlay_segments(void) const
  {
    if(!ovr)
      return 0;
    return ovr->segs.size();
  }

  //==== decode functions below ====

  Result<Header*, std::string> decode_hdr(segment<uint8_t> data)
  {
    // Get the header and perform magic check
    if(data.size() < sizeof(Exe))
      return std::string("Insufficient data for executable");
    Header* header = reinterpret_cast<Header*>(data.data());
    if(header->signature[0] != 'M' ||
        header->signature[1] != 'Z')
      return std::format<"Magic number mismatch: got {}{}, expected MZ">(header->signature[0], header->signature[1]);
    return header;
  }

  FBOV* decode_fbov(void* data)
  {
    // Get the struct and perform magic check
    FBOV* header = static_cast<FBOV*>(data);
    if(header->signature[0] != 'F' ||
        header->signature[1] != 'B' ||
        header->signature[2] != 'O' ||
        header->signature[3] != 'V')
      return nullptr;
    return header;
  }

  Result<Exe, std::string> Exe::decode(segment<uint8_t> data)
  {
    Exe rval;
    rval.rawdata = data;

    auto hdr = decode_hdr(data); // Decode the header
    if(hdr.is_err())
      return hdr.error();
    rval.hdr = *hdr;

    // Compute the EXE Region
    rval.exe_start = sizeof(paragraph_t) * hdr->header_paragraphs;
    rval.exe_end = sizeof(block_t) * hdr->blocks_in_file;

    if(hdr->bytes_in_last_block != 0)
      rval.exe_end -= 512 - uint32_t(hdr->bytes_in_last_block);

    if(rval.exe_end > data.size())
      return std::string("End of exe region is beyond the end of data");

    // Determine the relocs array
    rval.relocs = { data.ptr_at(hdr->reloc_table_offset), hdr->num_relocs };

    // Optional FBOV
    rval.fbov = decode_fbov(data.ptr_at(rval.exe_end));

    // Optional seginfo
    if(rval.fbov != nullptr)
    {
      if(rval.fbov->segnum < 0)
        return std::format<"Negative FBOV segnum: {}">(int32_t(rval.fbov->segnum));
      rval.seginfo = { data.ptr_at(rval.fbov->exeinfo), size_t(rval.fbov->segnum) };

      // Optional overlay info
      auto v = overlay::decode_overlay_info(data, rval.exe_start, rval.fbov, rval.seginfo);
      if(v.is_err())
        return v.error();
      rval.ovr = *v;
    }
    return rval;
  }
}
