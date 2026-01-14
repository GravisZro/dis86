#pragma once
#include <cstdint>
#include <array>
#include <map>

#include "instr_fmt.h"
#include "segoff.h"


namespace instr
{
  using namespace segoff;
  using namespace instr_fmt;

  enum class Rep_e
  {
    NE = 0,
    EQ,
  };

  enum class Size_e
  {
    Size8,
    Size16,
    Size32,
  };

  std::string_view to_str(Size_e val);

  struct RegInfo_t
  {
    const std::string_view name;
    Size_e sz;
    bool seg;
  };
  extern const std::array<RegInfo_t, 22> info;

  struct Register_t
  {
    enum e : uint8_t
    {
      AX=0,CX,  DX,  BX,
      SP,  BP,  SI,  DI,
      AL,  CL,  DL,  BL,
      AH,  CH,  DH,  BH,
      ES,  CS,  SS,  DS,
      IP,  FLAGS,
    } value = AX;

    Register_t(e v = AX) : value(v) { }
    Register_t(uint16_t v) : value(e(v)) { }

    operator e (void) {  return value; }

    static e reg8(uint8_t num);
    static e reg16(uint8_t num);
    static e sreg16(uint8_t num);

    std::string_view name(void) const;

    static std::optional<e> from_str_upper(const std::string& str);
  };
  static_assert(sizeof(Register_t) == 1);

  struct OperandReg_t
  {
    Register_t reg;
  };


  struct OperandMem_t
  {
    Size_e sz;
    Register_t sreg;
    std::optional<Register_t> reg1;
    std::optional<Register_t> reg2;
    std::optional<uint16_t> off;
  };

  struct OperandImm_t
  {
    Size_e sz;
    uint16_t val;
  };

  struct OperandRel_t
  {
    uint16_t val;
  };

  struct OperandFar_t
  {
    uint16_t seg;
    uint16_t off;
  };

  struct Operand_t
  {
    enum type_e : uint8_t
      { None = 0, Reg, Mem, Imm, Rel, Far };

    Operand_t(void) : type(None) { }
    Operand_t(const OperandReg_t& r) : type(Reg), reg(r) {}
    Operand_t(const OperandMem_t& m) : type(Mem), mem(m) {}
    Operand_t(const OperandImm_t& i) : type(Imm), imm(i) {}
    Operand_t(const OperandRel_t& r) : type(Rel), rel(r) {}
    Operand_t(const OperandFar_t& f) : type(Far), far(f) {}

    operator type_e(void) const { return type; }

    type_e type;
    union
    {
      OperandReg_t reg;
      OperandMem_t mem;
      OperandImm_t imm;
      OperandRel_t rel;
      OperandFar_t far;
    };
  };


  struct Instruction_t
  {
    std::optional<Rep_e> rep;
    operation_e opcode;
    std::array<Operand_t, 3> operands;
    SegOff_t  addr;
    uint16_t  n_bytes;
    uint8_t   intel_hidden_operand_bitmask; // bitmap of operands hidden in intel assembly

    SegOff_t end_addr(void) const
      { return addr.add_offset(n_bytes); }

    SegOff_t rel_addr(OperandRel_t rel) const
      { return end_addr().add_offset(rel.val); }

    std::string to_str(void) const;
  };
}
