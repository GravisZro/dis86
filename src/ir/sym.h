#pragma once

#include "common/print.h"
#include "asm/instr.h"
#include "config.h"
#include "types.h"

namespace ir::sym
{
  using namespace bin::instr;
  using namespace config;
  using types::Type;

  enum Table
  {
    Param,
    Local,
    Global,
    Register,
  };

  // Id is a reference to lookup the cooresponding Symbol
  struct Id
  {
    Table table;
    std::size_t idx;

    constexpr bool operator <(const Id& o) const
    {
      if(table != o.table)
        return table < o.table;
      return idx < o.idx;
    }

    constexpr bool operator ==(const Id& o) const
    {
      return table == o.table &&
             idx == o.idx;
    }
  };

  // Region is information about a byte range region
  struct Region
  {
    int32_t off;
    uint16_t sz;

    constexpr bool operator <(const Region& o) const
    {
      if(off != o.off)
        return off < o.off;
      return sz < o.sz;
    }
    constexpr bool operator ==(const Region& o) const
      { return off == o.off && sz == o.sz; }
  };

  struct SymbolDef
  {
    std::string name;
    Type typ;
    int16_t off;
    uint16_t size;

    constexpr int32_t start() const { return off; }
    constexpr int32_t end() const { return start() + size; }
  };

  struct SymbolTable
  {
    std::vector<SymbolDef> symbols; // Ordered by offset

    void append(const std::string& name, Type typ, int16_t off, uint16_t size)
      { symbols.push_back(SymbolDef { name, typ, off, size, }); }

      void coalesce(void);
      void finalize_non_overlaping(void);
  };

  struct SymbolMap;

  struct SymbolRef
  {
    Id id;
    Region access_region;   // Access using this region with the symbol

    bool operator <(const SymbolRef& o) const
    {
      if(id != o.id)
        return id < o.id;
      return access_region < o.access_region;
    }

    bool operator ==(const SymbolRef& o) const
      { return id == o.id && access_region == o.access_region; }

    SymbolDef& def(SymbolMap& map);
    const SymbolDef& def(const SymbolMap& map) const;
    constexpr Table table(void) const { return id.table; }

    std::string name(SymbolMap& map) const;
    constexpr int32_t off(void) const { return access_region.off; }
    constexpr uint16_t sz(void) const { return access_region.sz; }

    Type& get_type(SymbolMap& map);

    std::optional<SymbolRef> join_adjacent(SymbolMap& map, SymbolRef low, SymbolRef high);
  };

  struct SymbolMap
  {
    SymbolTable params;
    SymbolTable locals;
    SymbolTable globals;
    SymbolTable registers;

    SymbolMap(void);
    SymbolTable get_table(Table table) const;
    std::optional<SymbolRef> find_ref(Table table, int16_t off, uint16_t sz) const;

    std::optional<SymbolRef> find_ref_by_name(Table table, std::string name) const;
  };
/*
  void symbolize_stack(IR& ir);
  void populate_globals(IR& ir, const Config& cfg);
  void symbolize_globals(IR& ir, const Config& cfg);
  void symbolize(IR& ir, const Config& cfg);
*/
}
