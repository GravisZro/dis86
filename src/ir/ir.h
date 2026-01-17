#pragma once
#include <cstdint>
#include <vector>
#include <optional>
#include <string>

#include "config.h"
#include "defs.h"

namespace ir
{
  using namespace defs;

  struct IR
  {
    std::vector<int16_t> consts;
    SymbolMap symbols;
    std::vector<std::string> funcs;
    std::map<Ref, FullName> names;
    std::map<Name, std::size_t> name_next;
    std::vector<std::optional<Block>> blocks; // Optional because dead blocks can be pruned out

    void symbolize_stack(void);
    void populate_globals(const Config& cfg);
    void symbolize_globals(const Config& cfg);
    void symbolize(const Config& cfg);

          std::optional<Block>& block(BlockRef blkref)       { return blocks[blkref]; }
    const std::optional<Block>& block(BlockRef blkref) const { return blocks[blkref]; }

    BlockRef push_block(const Block& blk);
    void remove_block(BlockRef blkref);
    std::vector<BlockRef> iter_blocks(void) const;
    std::vector<Ref> iter_instrs(BlockRef blk);
    std::optional<Ref> prev_ref_in_block(const Ref& r) const;
    std::optional<Ref> next_ref_in_block(const Ref& r) const;

    Instr& get_instr(const Ref& r);
    std::optional<Instr> instr(const Ref& r) const;
    bool instr_matches(const Ref& r, Opcode op) const;
    Ref append_const(int16_t val);

    void phi_populate(const Name& sym, Ref phiref);
    Ref phi_create(const Name& sym, BlockRef blk);

    Ref get_var(const Name& sym, BlockRef blk);
    void set_var(const Name& sym, BlockRef blk, Ref r);
    void set_name(const Name& name, Ref r);

    void seal_block(BlockRef r);
    void unseal_all_blocks(void);
    void seal_all_blocks(void);
  };
}
