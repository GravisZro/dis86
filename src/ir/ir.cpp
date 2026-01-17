#include "ir.h"


namespace ir
{
  using namespace defs;

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

  std::optional<int16_t> lookup_const(Ref k)
  {
    if(k.type == Ref::Const)
      return k.m_const;
    return {};
  }

  void alter_instruction(Instr& ins, const SymbolRef& sym)
  {
    if(is_load(ins.opcode))
    {
      if(ins.opcode == Opcode::Load8)
        ins.opcode = Opcode::ReadVar8;
      else if(ins.opcode == Opcode::Load16)
        ins.opcode = Opcode::ReadVar16;
      else if(ins.opcode == Opcode::Load32)
        ins.opcode = Opcode::ReadVar32;
      else
        panic("! unreachable !");
      ins.operands = { Ref{ sym } };
    }
    else if(is_store(ins.opcode))
    {
      if(ins.opcode == Opcode::Store8)
        ins.opcode = Opcode::WriteVar8;
      else if(ins.opcode == Opcode::Store16)
        ins.opcode = Opcode::WriteVar16;
      else if(ins.opcode == Opcode::Store32)
        ins.opcode = Opcode::WriteVar32;
      else
        panic("! unreachable !");
      ins.operands = { Ref{ sym }, ins.operands[2] };
    }
  }


  void IR::symbolize_stack(void)
  {
    const Ref ss = { Register_t::SS };
    const Ref sp = { Register_t::SP };

    // Detect locals and params
    using var_mem_ref_t = std::tuple<Ref, Table, int16_t, std::size_t>;
    std::vector<var_mem_ref_t> var_mem_refs;

    for(BlockRef b : iter_blocks())
    {
      for(Ref& r : iter_instrs(b))
      {
        auto mem_ref = r;
        auto mem_instr = instr(mem_ref);
        assert(mem_instr);

        if(!is_load(mem_instr->opcode) && !is_store(mem_instr->opcode))
          continue;

        if(mem_instr->operands[0] != ss)
          continue;

        Ref addr_ref = mem_instr->operands[1];
        auto addr_instr = instr(addr_ref);
        if(!addr_instr || addr_instr->operands[0] != sp)
          continue;

        std::optional<int16_t> off;
        if(addr_instr->opcode == Opcode::Add ||
           addr_instr->opcode == Opcode::Sub)
          off = lookup_const(addr_instr->operands[1]);

        if(off && addr_instr->opcode == Opcode::Sub)
          off = -*off;

        if(!off)
          continue;

        std::size_t size = operation_size(mem_instr->opcode);

        // let mut f = crate::ir::display::Formatter::new();
        // f.fmt_instr(ir, addr_ref, addr_instr).unwrap();
        // f.fmt_instr(ir, mem_ref, mem_instr).unwrap();
        // println!("{}", f.finish());

        Type typ = infer_type_from_size(size);

        std::size_t frame_offset = 2;
        if(*off > 0)
        {
          auto name = std::format("_param_{:04x}", *off + frame_offset);
          symbols.params.append(name, typ, *off, size);
          var_mem_refs.emplace_back(mem_ref, Table::Param, *off, size);
        }
        else
        {
          auto name = std::format("_local_{:04x}", -(*off + frame_offset));
          symbols.locals.append(name, typ, *off, size);
          var_mem_refs.emplace_back(mem_ref, Table::Local, *off, size);
        }
      }
    }

    // Coalesce any duplicates or overlaps
    symbols.params.coalesce();
    symbols.locals.coalesce();

    // Update the IR
    for(auto [mem_ref, region, off, sz] : var_mem_refs)
    {
      auto sym = symbols.find_ref(region, off, sz);
      assert(sym);
      alter_instruction(get_instr(mem_ref), *sym);
    }
  }

  void IR::populate_globals(const Config& cfg)
  {
    for(const auto& g : cfg.globals)
    {
      // FIXME: Remove the Type::Unknown
      auto size = g.typ.size_in_bytes();
      if(!size)
      {
        eprintln("WARN: Unsupported type '{}' for {} ... assuming u32", g.typ.to_string(), g.name);
        size = 4;
      }
      symbols.globals.append(g.name, g.typ, g.offset, *size);
    }
    symbols.globals.finalize_non_overlaping();
  }

  void IR::symbolize_globals(const Config& cfg)
  {
    populate_globals(cfg);

    Ref ds = { Register_t::DS };

    for(auto& b : iter_blocks())
    {
      for(Ref& r : iter_instrs(b))
      {
        auto ins = instr(r);
        assert(ins);

        if(!is_load(ins->opcode) && !is_store(ins->opcode))
          continue;

        if(ins->operands[0] != ds)
          continue;

        Ref& off_ref = ins->operands[1];
        std::size_t size = operation_size(ins->opcode);
        auto off = lookup_const(off_ref);
        if(!off)
          continue;

        auto sym = symbols.find_ref(Table::Global, *off, size);
        if(!sym)
        {
          eprintln("WARN: Could not find global for DS:{:04x}", *off);
          continue;
        };

        alter_instruction(get_instr(r), *sym);
      }
    }
  }

  void IR::symbolize(const Config& cfg)
  {
    symbolize_stack();
    symbolize_globals(cfg);
  }


  BlockRef IR::push_block(const Block& blk)
  {
    BlockRef idx = blocks.size();
    blocks.push_back(blk);
    return idx;
  }

  void IR::remove_block(BlockRef blkref)
  {
    // Caller is responsible for ensuring there are no references to this block elsewhere in the IR
    assert(blocks[blkref]);
    blocks[blkref].reset();
  }

  std::vector<BlockRef> IR::iter_blocks(void) const
  {
    // FIXME: Can we avoid the intermediate vec?? (Without holding &self hostage?)
    std::vector<BlockRef> blkrefs;
    for(std::size_t i = 0; i < blocks.size(); ++i)
      if(blocks[i])
        blkrefs.push_back(BlockRef(i));
    return blkrefs;
  }

  std::vector<Ref> IR::iter_instrs(BlockRef blk)
  {
    assert(block(blk));
    std::vector<Ref> refs;
    auto& ins = block(blk)->instrs;
    const auto& rng = ins.range();
    for(int64_t i = rng.start; i < rng.end; ++i)
      refs.emplace_back(std::pair<BlockRef, DVecIndex>(blk, i));
    return refs;
  }

  std::optional<Ref> IR::prev_ref_in_block(const Ref& r) const
  {
    if(r.type != Ref::Instr)
      return {};
    auto& b = r.m_instr.first;
    const auto& blk = block(b);
    if(blk)
      for(auto i = r.m_instr.second; i < blk->instrs.start(); --i)
        if(blk->instrs[i].opcode != Opcode::Nop)
          return Ref(std::pair<BlockRef, DVecIndex>(b, i));
    return {};
  }

  std::optional<Ref> IR::next_ref_in_block(const Ref& r) const
  {
    if(r.type != Ref::Instr)
      return {};
    auto& b = r.m_instr.first;
    const auto& blk = block(b);
    if(blk)
      for(auto i = r.m_instr.second; i < blk->instrs.start(); ++i)
        if(blk->instrs[i].opcode != Opcode::Nop)
          return Ref(std::pair<BlockRef, DVecIndex>(b, i));
    return {};
  }

  Instr& IR::get_instr(const Ref& r)
  {
    assert(r.type == Ref::Instr);
    return block(r.m_instr.first)->instrs[r.m_instr.second];
  }

  std::optional<Instr> IR::instr(const Ref& r) const
  {
    if(r.type != Ref::Instr)
      return {};
    return block(r.m_instr.first)->instrs[r.m_instr.second];
  }

  bool IR::instr_matches(const Ref& r, Opcode op) const
  {
    std::optional<Instr> ins = instr(r);
    return ins && ins->opcode == op;
  }


  Ref IR::append_const(int16_t val)
  {
    // Search existing constants for a duplicate (TODO: Make this into a hash-tbl if it gets slow)
    for(std::size_t i = 0; i < consts.size(); ++i)
      if(val == consts[i])
        return Ref(Ref::Const, i);

    // Add new constant
    std::size_t idx = consts.size();
    consts.push_back(val);
    return Ref(Ref::Const, idx);
  }

  void IR::set_name(const Name& name, Ref r)
  {
    size_t idx = 0;
    auto iter = name_next.find(name);
    if(iter == std::end(name_next))
    {
      name_next.emplace(name, 1);
      idx = 1;
    }
    else
      idx = iter->second + 1;

    names.emplace(r, FullName { name, idx });
  }

  void IR::set_var(const Name& sym, BlockRef blk, Ref r)
  {
    auto& blkval = block(blk);
    assert(blkval);
    blkval->defs.emplace(sym, r);
    set_name(sym, r);
  }

  Ref IR::phi_create(const Name& sym, BlockRef blk)
  {
    // create phi node (without operands) to terminate recursion
    auto& blkval = block(blk);
    assert(blkval);
    auto idx = blkval->instrs.push_front(Instr {
      Type::U16, // TODO: SANITY CHECK THAT NO OTHER SIZES CAN GO THROUGH A PHI!!
      Attribute::NONE,
      Opcode::Phi,
        {}
    });

    auto vref = Ref({blk, idx});
    set_var(sym, blk, vref);
    return vref;
  }

  Ref IR::get_var(const Name& sym, BlockRef blk)
  {
    // Defined locally in this block? Easy.
    auto& blkval = block(blk);
    assert(blkval);
    auto& defs = block(blk)->defs;
    if(auto iter = defs.find(sym); iter != std::end(defs))
      return iter->second;

    // Otherwise, search predecessors
    if(!blkval->sealed)
    {
        // add an empty phi node and mark it for later population
        auto phi = phi_create(sym, blk);
        blkval->incomplete_phis.emplace_back(std::pair<Name, Ref>{ sym, phi });
        return phi;
    }
    else
    {
      auto& preds = blkval->preds;;
      if(preds.size() == 1)
        return get_var(sym, preds[0]);
      else
      {
        // create a phi and immediately populate it
        auto phi = phi_create(sym, blk);
        phi_populate(sym, phi);
        return phi;
      }
    }
  }


  void IR::phi_populate(const Name& sym, Ref phiref)
  {
    auto [blk, idx] = phiref.unwrap_instr();

    auto blkdata = block(blk);
    if(!blkdata)
      return;

    auto preds = blkdata->preds; // ARGH: Need to break borrow on 'self' so we can recurse
    assert(block(blk)->instrs[idx].opcode == Opcode::Phi);

    // recurse each pred
    std::vector<Ref> refs;
    for(auto& b : preds)
      refs.push_back(get_var(sym, b));

    // update the phi with operands
    block(blk)->instrs[idx].operands = refs;

    // TODO: Remove trivial phis
  }

  void IR::seal_block(BlockRef r)
  {
    auto& b = block(r);
    assert(b);
    if(b->sealed)
      panic("block is already sealed!");
    b->sealed = true;
    for(auto& pair : b->incomplete_phis)
    {
      auto [sym, phi] = std::move(pair);
      phi_populate(sym, phi);
    }
  }

  void IR::unseal_all_blocks(void)
  {
    for(auto& b : iter_blocks())
      if(auto& v = block(b); v)
        v->sealed = false;
  }

  void IR::seal_all_blocks(void)
  {
    for(auto& b : iter_blocks())
      seal_block(b);
  }
}
