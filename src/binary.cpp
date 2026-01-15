#include "binary.h"


#include "common/fileio.h"
#include "common/print.h"


namespace binary
{
  using namespace segoff;
  using region::RegionIter_t;
  using config::Config;

  std::vector<uint16_t> build_segmap(const binfmt::mz::Exe& exe)
  {
    std::vector<uint16_t> rval;
    const auto& segmap = exe.seginfo;
    if(segmap.valid())
      for(std::size_t i = 0; i < segmap.size(); ++i)
        rval.push_back(segmap[i].seg);
    return rval;
  }

  Result<Binary, std::string> Binary::from_fmt(const Fmt& fmt, const std::optional<Config>& config)
  {
    auto data = read_file<std::vector<uint8_t>>(fmt.path);
    if(data.is_err())
      return std::format("Failed to read file: '{}': {:?}",
                         static_cast<std::string>(fmt.path),
                         data.error().message());


    if(fmt == Fmt::Raw)
      return from_raw(std::move(*data), config);
    else if(fmt != Fmt::Exe)
      panic("invalid executable format");

    auto exe = binfmt::mz::Exe::decode(std::move(*data));
    if(exe.is_err())
      return exe.error();
    return from_exe(*exe, config);
  };

  Result<Binary, std::string> Binary::from_exe(
      binfmt::mz::Exe& exe,
      const std::optional<Config>& config)
  {
    Binary rval;
    rval.main = exe.exe_data();
    rval.config = config;
    rval.rawdata = std::move(exe.rawdata);

    for(std::size_t i = 0; i < exe.num_overlay_segments(); ++i)
      rval.overlays.push_back(exe.overlay_data(i));

    rval.segmap = build_segmap(exe);
    if(rval.segmap->empty())
      rval.segmap.reset();

    return rval;
  }

  segment<uint8_t> Binary::region(SegOff_t start, SegOff_t end) const
  {
    assert(start.seg != end.seg);
    if(start.seg == Seg_t::Normal)
      return segment<uint8_t>(main.ptr_at(start.abs_normal()), end.abs_normal() - start.abs_normal());
    else if(start.seg != Seg_t::Overlay)
      panic("Invalid segment enumeration value");

    return segment<uint8_t> {
        overlays[start.seg.operator uint16_t()].ptr_at(start.off),
        std::size_t(end.off - start.off) };
  }

  Seg_t Binary::remap_to_segment(uint16_t old) const
  {
    if(!segmap)
      panic("Cannot remap segments when binary has no seginfo table");
    assert(old % 8 == 0);
    return Seg_t { Seg_t::Normal, segmap->at(old / 8) };
  }

  std::optional<config::Func> Binary::lookup_call(SegOff_t from, SegOff_t to) const
  {
    if(config)
    {
      if(from.seg == Seg_t::Normal)
        return config->func_lookup(to);
      if(from.seg == Seg_t::Overlay && to.seg == Seg_t::Normal)
        return config->func_lookup({ remap_to_segment(to.seg), to.off });
    }
    return {};
  }
}
