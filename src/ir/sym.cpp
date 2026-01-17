#include "sym.h"

#include "defs.h"

#include <algorithm>

namespace ir::sym
{
  using namespace bin::instr;
  using namespace config;
  using namespace ir::defs;
  using types::Type;

  Type infer_type_from_size(uint16_t size)
  {
    if(size == 1)
      return Type::U8;
    if(size == 2)
      return Type::U16;
    if(size == 3)
      return Type::U32;
    panic("Unsupported size");
    return {};
  }


  void SymbolTable::coalesce(void)
  {
    if(!symbols.empty())
    {
      std::sort(std::begin(symbols), std::end(symbols),
                [](const SymbolDef& a, const SymbolDef& b)
                {
                  if(a.off == b.off)
                    return a.size < b.size;
                  return a.off < b.off;
                });

      std::vector<SymbolDef> new_symbols = { symbols.at(0) };
      for(std::size_t i = 1; i < symbols.size(); ++i)
      {
        auto& sym = symbols[i];
        auto last_idx = new_symbols.size() - 1;
        auto& last = new_symbols[last_idx];

        if(sym.start() < last.end()) // overlapping?
        {
          // simply update the last size
          last.size = (sym.end() - last.start());
          // also update the type
          last.typ = infer_type_from_size(last.size);
        }
        else // disjoint?
          new_symbols.push_back(sym);
      }

      symbols = new_symbols;
    }
  }

  void SymbolTable::finalize_non_overlaping()
  {
    std::sort(std::begin(symbols), std::end(symbols),
              [](const SymbolDef& a, const SymbolDef& b)
              {
                if(a.off == b.off)
                  return a.size < b.size;
                return a.off < b.off;
              });

    // FIXME: ADD THIS BACK
    // for idx in 1..symbols.len() {
    //   if symbols[idx].start() < symbols[idx-1].end() { // overlapping
    //     panic!("Overlapping symbols: {} and {}", symbols[idx-1].name, symbols[idx].name);
    //   }
    // }
  }


  SymbolDef& SymbolRef::def(SymbolMap& map)
    { return map.get_table(id.table).symbols[id.idx]; }

  const SymbolDef& SymbolRef::def(const SymbolMap& map) const
    { return map.get_table(id.table).symbols[id.idx]; }


  std::string SymbolRef::name(SymbolMap& map) const
  {
    if(!off())
      return def(map).name;
    return std::format("{}@+{}", def(map).name, off());
  }

  Type& SymbolRef::get_type(SymbolMap& map) { return def(map).typ; }

  std::optional<SymbolRef> join_adjacent(SymbolMap& map, SymbolRef low, SymbolRef high)
  {
    /*
    let low_sym = low.def(map);
    let high_sym = high.def(map);
    if low_sym as *const _ != high_sym as *const _ {
      return None;
    }
    let low_endoff = low.off() + low.sz() as i32;
    if high.off() as i32 != low_endoff {
      return None;
    }
    Some(SymbolRef {
      id: low.id,
      access_region: Region {
        off: low.access_region.off,
        sz: low.access_region.sz + high.access_region.sz,
      }
    })
*/
    return {};
  }


  SymbolMap::SymbolMap(void)
  {
      // Add all registers
    registers.append("AX",    Type::U16,  0, 2);
    registers.append("CX",    Type::U16,  2, 2);
    registers.append("DX",    Type::U16,  4, 2);
    registers.append("BX",    Type::U16,  6, 2);
    registers.append("SP",    Type::U16,  8, 2);
    registers.append("BP",    Type::U16, 10, 2);
    registers.append("SI",    Type::U16, 12, 2);
    registers.append("DI",    Type::U16, 14, 2);
    registers.append("ES",    Type::U16, 16, 2);
    registers.append("CS",    Type::U16, 18, 2);
    registers.append("SS",    Type::U16, 20, 2);
    registers.append("DS",    Type::U16, 22, 2);
    registers.append("IP",    Type::U16, 24, 2);
    registers.append("FLAGS", Type::U16, 26, 2);
  }

  SymbolTable SymbolMap::get_table(Table table) const
  {
    if(table == Table::Param)
      return params;
    if(table == Table::Local)
      return locals;
    if(table == Table::Global)
      return globals;
    if(table != Table::Register)
      panic("! Invalid enumeration value !");
    return registers;
  }

  std::optional<SymbolRef> SymbolMap::find_ref(Table table, int16_t off, uint16_t sz) const
  {
      // FIXME: This is sorted: can use binary search
      auto tbl = get_table(table);
      for(size_t i = 0; i < tbl.symbols.size(); ++i)
      {
        auto& sym = tbl.symbols[i];
        if(sym.start() <= off && off < sym.end())
          return SymbolRef { Id { table, i },
                            Region { off - sym.start(), sz } };
      }
      return {};
    }

  std::optional<SymbolRef> SymbolMap::find_ref_by_name(Table table, std::string name) const
  {
    auto tbl = get_table(table);
    for(size_t i = 0; i < tbl.symbols.size(); ++i)
    {
      auto& sym = tbl.symbols[i];
      if(sym.name == name)
        return SymbolRef { Id { table, i },
                         Region { 0, sym.size } };
    }
    return {};
  }
}
