#pragma once

#include <cstdint>
#include <filesystem>

#include "types.h"
#include "segoff.h"
#include "asm/instr.h"
#include "bsl/bsl.h"

namespace config
{
  using namespace segoff;
  using bin::instr::Register_t;
  using namespace types;

  enum CallMode
  {
    Near,
    Far,
  };

  struct OverlayRange
  {
    uint16_t num;
    uint16_t start;
    uint16_t end;
  };

  struct Func
  {
    std::string name;
    SegOff_t start;
    std::optional<SegOff_t> end;
    std::optional<OverlayRange> overlay;
    CallMode mode;
    Type ret;
    std::optional<uint16_t> args;  // None means "unknown", Some(0) means "no args"
    std::optional<std::vector<Register_t>> regargs;
    bool dont_pop_args;
  };

  struct Indirect
  {
    SegOff_t addr;
    Type ret;
    uint16_t args;
  };

  struct Global
  {
    std::string name;
    uint16_t offset;
    Type typ;
  };

  struct TextSectionRegion
  {
    std::string name;
    SegOff_t start;
    SegOff_t end;
    Type typ;
    std::optional<SegOff_t> access;
  };


  class Config
  {
  public:
    static Result<Config, std::string> from_path(const std::filesystem::path& path);

    types::Builder type_builder;
    std::vector<Struct> structs;
    std::vector<Func> funcs;
    std::vector<Indirect> indirects;
    std::vector<Global> globals;
    std::vector<TextSectionRegion> text_section;

    std::optional<Func> func_lookup(SegOff_t addr) const;
    std::optional<Indirect> indirect_lookup(SegOff_t addr) const;
    std::optional<Func> func_lookup_by_name(const std::string& name) const;
    std::optional<TextSectionRegion> text_region_lookup_by_start_addr(SegOff_t addr) const;
    std::optional<TextSectionRegion> text_region_lookup_by_access(SegOff_t addr) const;
    std::optional<TextSectionRegion> text_region_lookup(SegOff_t start_addr, SegOff_t access) const;

  private:
    Result<std::nullptr_t, std::string> parse_functions(std::shared_ptr<bsl::node_t> root);
    Result<std::nullptr_t, std::string> parse_structs(std::shared_ptr<bsl::node_t> root);
    Result<std::nullptr_t, std::string> parse_globals(std::shared_ptr<bsl::node_t> root);
    Result<std::nullptr_t, std::string> parse_text_section(std::shared_ptr<bsl::node_t> root);
  };
}
