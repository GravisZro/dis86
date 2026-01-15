#pragma once
#include <cstdint>
#include <vector>
#include <optional>
#include <string>

#include "common/segment.h"
#include "config.h"
#include "segoff.h"
#include "region.h"

#include "binfmt/mz/exe.h"

#include <filesystem>

namespace binary
{
  using namespace segoff;
  using region::RegionIter_t;
  using config::Config;

  struct Fmt
  {
    enum type_e
    {
      Raw = 0,
      Exe,
    } type;

    constexpr operator type_e(void) const { return type; }

    std::filesystem::path path;
  };

  struct Binary
  {
    segment<uint8_t> main;
    std::vector<segment<uint8_t>> overlays;
    std::optional<Config> config;
    std::optional<std::vector<uint16_t>> segmap;
    std::vector<uint8_t> rawdata;

    segment<uint8_t> region(SegOff_t start, SegOff_t end) const;

    RegionIter_t region_iter(SegOff_t start, SegOff_t end) const
      { return RegionIter_t { region(start, end), start }; }

    Seg_t remap_to_segment(uint16_t old) const;

    std::optional<config::Func> lookup_call(SegOff_t from, SegOff_t to) const;

    static Binary from_raw(std::vector<uint8_t>&& data, const std::optional<Config>& config)
      { return { { data.data(), data.size() }, {}, config, {}, data }; }

    static Result<Binary, std::string> from_fmt(const Fmt& fmt, const std::optional<Config>& config);
    static Result<Binary, std::string> from_exe(binfmt::mz::Exe& exe,
                                                const std::optional<Config>& config);
  };
}
