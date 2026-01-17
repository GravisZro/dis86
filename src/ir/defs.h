#pragma once

#include <cstdint>
#include "types.h"
#include "asm/instr.h"
#include "util/dvec.h"
#include "opcode.h"
#include "sym.h"
/*
use crate::bin::instr;
use crate::ir::sym;
use crate::types::Type;
use crate::util::dvec::{DVec, DVecIndex};
use std::collections::HashMap;
*/
namespace ir::defs
{
  using namespace opcode;
  using namespace dvec;
  using namespace ir::sym;
  using types::Type;
  using bin::instr::Register_t;

// SSA IR Definitions

  using ConstRef = std::size_t;
  using BlockRef = std::size_t;

  struct Ref
  {
    enum e
    {
      Const = 0,
      Instr,
      Init,
      Block,
      Symbol,
      Func,
    } type;

    Ref(Ref::e t, std::size_t val);
    Ref(std::pair<BlockRef, DVecIndex> instr) : type(Instr), m_instr(instr) { }
    Ref(Register_t init) : type(Init), m_init(init) { }
    Ref(SymbolRef sym) : type(Symbol), m_symbol(sym) { }

    constexpr bool is_const(void) const { return type == e::Const; }

    bool operator ==(const Ref& o) const;
    bool operator <(const Ref& o) const;

    BlockRef unwrap_block(void) const;
    SymbolRef unwrap_symbol(void) const;
    std::size_t unwrap_func(void) const;
    std::pair<BlockRef, DVecIndex> unwrap_instr(void) const;

    ConstRef    m_const;
    std::pair<BlockRef, DVecIndex> m_instr;
    Register_t  m_init;
    BlockRef    m_block;
    SymbolRef   m_symbol;
    std::size_t m_func;
  };



  struct Name
  {
    enum e : uint8_t
    {
      Reg = 0,
      Var,
    } type;

    Name(Register_t::e      r) : type(Reg), reg(r) { }
    Name(const Register_t&  r) : type(Reg), reg(r) { }
    Name(const std::string& v) : type(Var), var(v) { }

    constexpr bool operator ==(e t) const { return type == t; }
    bool operator <(const Name& o) const;
    bool operator ==(const Name& o) const;

    Register_t reg;
    std::string var;
  };

  struct FullName
  {
    Name name;
    std::size_t val;
  };

  enum Attribute : uint8_t
  {
    NONE = 0,
    MAY_ESCAPE = 1,
    STACK_PTR  = 2,
    PIN = 4,
  };


  struct Instr
  {
    Type typ;
    uint8_t attrs;
    Opcode opcode;
    std::vector<Ref> operands;
  };

  struct Block
  {
    Block(std::string n);
    std::vector<BlockRef> exits(void);

    std::string name;
    std::map<Name, Ref> defs;
    std::vector<BlockRef> preds;
    DVec<Instr> instrs;
    bool sealed; // has all predecessors?
    std::vector<std::pair<Name, Ref>> incomplete_phis;
  };
}
