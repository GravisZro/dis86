#include "exe.h"

#include <format>
#include "common/print.h"

namespace binfmt::mz
{
  using namespace std::string_literals;

  segment<uint8_t> Exe::exe_data(void) const
  {
    return segment<uint8_t> { rawdata.ptr_at(exe_start),
                            exe_end - exe_start };
  }

  segment<uint8_t> Exe::overlay_data(std::size_t id) const
  {
    assert(ovr);
    const OverlaySeg& seg = ovr->segs[id];
    std::size_t start = ovr->file_offset + seg.data_offset;
    std::size_t end = start + seg.segment_size;
    return segment<uint8_t> { rawdata.ptr_at(start),
                            end - start };
  }


  std::size_t Exe::num_overlay_segments(void) const
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
      return "Insufficient data for executable"s;
    Header* header = reinterpret_cast<Header*>(data.data());
    if(header->signature[0] != 'M' ||
        header->signature[1] != 'Z')
      return std::format("Magic number mismatch: got {}{}, expected MZ", header->signature[0], header->signature[1]);
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
      return "End of exe region is beyond the end of data"s;

    // Determine the relocs array
    rval.relocs = { data.ptr_at(hdr->reloc_table_offset), hdr->num_relocs };

    // Optional FBOV
    rval.fbov = decode_fbov(data.ptr_at(rval.exe_end));

    // Optional seginfo
    if(rval.fbov != nullptr)
    {
      if(rval.fbov->segnum < 0)
        return std::format("Negative FBOV segnum: {}", int32_t(rval.fbov->segnum));
      rval.seginfo = { data.ptr_at(rval.fbov->exeinfo), std::size_t(rval.fbov->segnum) };

      // Optional overlay info
      auto v = overlay::decode_overlay_info(data, rval.exe_start, rval.fbov, rval.seginfo);
      if(v.is_err())
        return v.error();
      rval.ovr = *v;
    }
    return rval;
  }

  //==== print functions below ====

  static std::string to_str(SegInfoType typ)
  {
    switch (typ)
    {
      case SegInfoType::DATA: return "DATA";
      case SegInfoType::CODE: return "CODE";
      case SegInfoType::STUB: return "STUB";
      case SegInfoType::OVERLAY: return "OVERLAY";
    }
    return "!! invalid value for SegInfoType !!";
  }

  void Exe::print_hdr(void) const
  {
    println("MZ Header:");
    println("  magic:    0x{:02x}{:02x} (\"{}{}\")", hdr->signature[0], hdr->signature[1], hdr->signature[0], hdr->signature[1]);
    println("  cblp      0x{:04x} ({})", hdr->bytes_in_last_block,  hdr->bytes_in_last_block);
    println("  cp        0x{:04x} ({})", hdr->blocks_in_file,       hdr->blocks_in_file);
    println("  crlc      0x{:04x} ({})", hdr->num_relocs,           hdr->num_relocs);
    println("  cparhdr   0x{:04x} ({})", hdr->header_paragraphs,    hdr->header_paragraphs);
    println("  minalloc  0x{:04x} ({})", hdr->min_extra_paragraphs, hdr->min_extra_paragraphs);
    println("  maxalloc  0x{:04x} ({})", hdr->max_extra_paragraphs, hdr->max_extra_paragraphs);
    println("  ss        0x{:04x} ({})", hdr->ss,                   hdr->ss);
    println("  sp        0x{:04x} ({})", hdr->sp,                   hdr->sp);
    println("  csum      0x{:04x} ({})", hdr->checksum,             hdr->checksum);
    println("  ip        0x{:04x} ({})", hdr->ip,                   hdr->ip);
    println("  cs        0x{:04x} ({})", hdr->cs,                   hdr->cs);
    println("  lfarlc    0x{:04x} ({})", hdr->reloc_table_offset,   hdr->reloc_table_offset);
    println("  ovno      0x{:04x} ({})", hdr->overlay_number,       hdr->overlay_number);
    println("");
    println("Exe Region:");
    println("  start     0x{:08x}", exe_start);
    println("  end       0x{:08x}", exe_end);
    println("");
  }

  void Exe::print_relocs(segment<Reloc> relocs) const
  {
    print("Relocations:");
    for(std::size_t i = 0; i < relocs.size(); ++i)
    {
      Reloc& r = relocs[i];
      if(i % 16 == 0)
        println("");
      print("  {:04x}:{:04x}", r.segment, r.offset);
    }
    println("");
    println("");
  }

  void Exe::print_fbov(FBOV* fbov) const
  {
    println("FBOV Header:");
    println("  magic:    0x{:02x}{:02x}{:02x}{:02x} (\"{}{}{}{}\")",
            fbov->signature[0], fbov->signature[1], fbov->signature[2], fbov->signature[3],
            fbov->signature[0], fbov->signature[1], fbov->signature[2], fbov->signature[3]);
    println("  ovrsize   0x{:08x} ({})",   fbov->ovrsize, fbov->ovrsize);
    println("  exeinfo   0x{:08x} ({})",   fbov->exeinfo, fbov->exeinfo);
    println("  segnum    0x{:08x} ({})",   fbov->segnum, fbov->segnum);
    println("");
  }

  void Exe::print_seginfo(segment<SegInfo> seginfo) const
  {
    println("Segment Information:");
    println("  {:<4}  {:<8}  {:<12}  {:<8}  {:<8}  {:<10}",
             "num", "seg", "type", "minoff", "maxoff", "size");

    for(std::size_t i = 0; i < seginfo.size(); ++i)
    {
      SegInfo& s = seginfo[i];
      std::string typ_str = std::format("{}({})", to_str(s.typ), int(s.typ));
      println(" {:4}   0x{:04x}    {:<12}  0x{:04x}    0x{:04x}    {:5} (0x{:04x})",
               i, s.seg, typ_str, s.minoff, s.maxoff, s.size(), s.size());
    }
    println("");
  }

  void Exe::print_overlayinfo(const overlay::OverlayInfo& ovr) const
  {
    println("Overlay File Offset: 0x{:x}", ovr.file_offset);
    println("");

    println("Overlay Segments:");
    println("   num      data_off    data_end    seg_size    _unknown_1    _unknown_2");
    for(std::size_t i = 0; i < ovr.segs.size(); ++i)
    {
      const OverlaySeg& seg = ovr.segs[i];
      uint32_t end = seg.data_offset + seg.segment_size;
      println("   {:3}   0x{:08x}   0x{:08x}   {:9}        0x{:04x}        0x{:04x}",
               i, seg.data_offset, end, seg.segment_size, seg._unknown_1, seg._unknown_2);
    }
    println("");

    println("Overlay Stubs:");
    for(const auto& stub : ovr.stubs)
      println("  {} => {}", stub.stub_addr().to_str(), stub.dest_addr().to_str());
  }

  void Exe::print_exe(void) const
  {
    print_hdr();
    print_relocs(relocs);
    if(fbov != nullptr)
      print_fbov(fbov);
    if(seginfo.valid())
      print_seginfo(seginfo);
    if(ovr)
      print_overlayinfo(*ovr);
  }
}
