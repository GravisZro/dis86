#include "defs.h"

#include <ranges>

namespace ir::defs
{

  Ref::Ref(Ref::e t, std::size_t val) : type(t)
  {
    if(type == Const)
      m_const = val;
    else if(type == Block)
      m_block = val;
    else if(type == Func)
      m_func = val;
  }

  bool Ref::operator ==(const Ref& o) const
  {
    if(type != o.type)
      return false;
    if(type == Const && o.type == Const)
      return m_const == o.m_const;
    if(type == Instr && o.type == Instr)
      return m_instr.first == o.m_instr.first;
    if(type == Init && o.type == Init)
      return m_init.value == o.m_init.value;
    if(type == Block && o.type == Block)
      return m_block == o.m_block;
    if(type == Symbol && o.type == Symbol)
      return m_symbol == o.m_symbol;
    if(type == Func && o.type == Func)
      return m_func == o.m_func;
    return false; // invalid type values!
  }

  bool Ref::operator <(const Ref& o) const
  {
    if(type != o.type)
      return type < o.type;
    if(type == Const && o.type == Const)
      return m_const < o.m_const;
    if(type == Instr && o.type == Instr)
      return m_instr.first < o.m_instr.first;
    if(type == Init && o.type == Init)
      return m_init.value < o.m_init.value;
    if(type == Block && o.type == Block)
      return m_block < o.m_block;
    if(type == Symbol && o.type == Symbol)
      return m_symbol < o.m_symbol;
    if(type == Func && o.type == Func)
      return m_func < o.m_func;
    return false; // invalid type values!
  }


  BlockRef Ref::unwrap_block(void) const
  {
    if(type != e::Block)
      panic("expected block ref");
    return m_block;
  }

  SymbolRef Ref::unwrap_symbol(void) const
  {
    if(type != e::Symbol)
      panic("expected symbol ref");
    return m_symbol;
  }

  std::size_t Ref::unwrap_func(void) const
  {
    if(type != e::Func)
      panic("expected function ref");
    return m_func;
  }

  std::pair<BlockRef, DVecIndex> Ref::unwrap_instr(void) const
  {
    if(type != e::Instr)
      panic("expected instr ref");
    return m_instr;
  }


  bool Name::operator <(const Name& o) const
  {
    if(type == Reg && o.type == Reg)
      return reg.value < o.reg.value;
    if(type == Var && o.type == Var)
      return var < o.var;
    return type < o.type;
  }

  bool Name::operator ==(const Name& o) const
  {
    if(type == Reg && o.type == Reg)
      return reg.value == o.reg.value;
    if(type == Var && o.type == Var)
      return var == o.var;
    return type == o.type;
  }



  Block::Block(std::string n)
      : name(n),
        sealed(false)
  {}

  std::vector<BlockRef> Block::exits(void)
  {
    auto instr = instrs.last();
    assert(instr);
    switch(instr->opcode)
    {
      case Opcode::RetFar:
      case Opcode::RetNear:
        break;
      case Opcode::Jmp: return { instr->operands[0].unwrap_block() };
      case Opcode::Jne:
        return { instr->operands[1].unwrap_block(),
                 instr->operands[2].unwrap_block()};
      case Opcode::JmpTbl:
      {
        std::vector<BlockRef> rval;
        for(std::size_t i = 1; i < instr->operands.size(); ++i)
          rval.push_back(instr->operands[i].unwrap_block());
        return rval;
      }
      default:
        panic("Expected last instruction to be a branching instruction: {:?}", ""); // instr.value()

    }
    return {};
  }

}
