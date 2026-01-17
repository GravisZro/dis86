#pragma once
#include <string_view>

#include "common/print.h"

namespace opcode
{
  enum class Opcode : uint8_t
  {
    Nop = 0, Pin,  Ref,  Phi,  Unimpl,

    Add,  Sub,  Shl,
    Shr,  UShr,  // signed/unsigned
    And,  Or,  Xor,
    IMul, UMul,  // signed/unsigned
    IDiv, UDiv,  // signed/unsigned

    Neg,  // logical
    Not,  // bitwise

    SignExtTo32,

    Load8,      Load16,     Load32,
    Store8,     Store16,    Store32,
    ReadVar8,   ReadVar16,  ReadVar32,
    WriteVar8,  WriteVar16, WriteVar32,
    ReadArr8,   ReadArr16,  ReadArr32,
    WriteArr8,  WriteArr16,

    Lower16, // uint16_t Lower16(uint32_t n) { return n & 0xFFFF; }
    Upper16, // uint16_t Upper16(u32 n) { return (n >> 16) & 0xFFFF; }
    Make32,  // uint32_t Make32(uint16_t high, uint16_t low) { return (high << 16) | low; }

    UpdateFlags,
    EqFlags,     // Maps to: JE  / JZ
    NeqFlags,    // Maps to: JNE / JNZ
    GtFlags,     // Maps to: JG  / JNLE
    GeqFlags,    // Maps to: JGE / JNL
    LtFlags,     // Maps to: JL  / JNGE
    LeqFlags,    // Maps to: JLE / JNG
    UGtFlags,    // Maps to: JA  / JNBE
    UGeqFlags,   // Maps to: JAE / JNB  / JNC
    ULtFlags,    // Maps to: JB  / JNAE / JC
    ULeqFlags,   // Maps to: JBE / JNA
    SignFlags,   // Maps to: JS and inverted for JNS,

    Eq,          // Operator: == (any sign)
    Neq,         // Operator: != (any sign)
    Gt,          // Operator: >  (signed)
    Geq,         // Operator: >= (signed)
    Lt,          // Operator: <  (signed)
    Leq,         // Operator: <= (signed)
    UGt,         // Operator: >  (unsigned)
    UGeq,        // Operator: >= (unsigned)
    ULt,         // Operator: <  (unsigned)
    ULeq,        // Operator: <=  (unsigned)
    Sign,        // Is Signed?
    NotSign,     // Is not signed?

    CallFar,
    CallNear,
    CallPtr,
    CallArgs,
    Int,  // invoke interrupt

    RetFar,
    RetNear,

    Jmp,
    Jne,
    JmpTbl,

    // TODO: HMMM.... Better Impl?
    AssertEven,
    AssertPos,
  };

  extern std::string_view to_string(Opcode value);

  constexpr bool is_load(Opcode o)
  {
    return o == Opcode::Load8 ||
           o == Opcode::Load16 ||
           o == Opcode::Load32;
  }

  constexpr bool is_store(Opcode o)
  {
    return o == Opcode::Store8 ||
           o == Opcode::Store16 ||
           o == Opcode::Store32;
  }

  constexpr bool is_read_var(Opcode o)
  {
    return o == Opcode::ReadVar8 ||
           o == Opcode::ReadVar16 ||
           o == Opcode::ReadVar32;
  }

  constexpr bool is_write_var(Opcode o)
  {
    return o == Opcode::WriteVar8 ||
           o == Opcode::WriteVar16 ||
           o == Opcode::WriteVar32;
  }


  constexpr bool is_mem_op(Opcode o)
  {
    return is_load(o) ||
           is_store(o) ||
           is_read_var(o) ||
           is_write_var(o);
  }

  constexpr bool is_call(Opcode o)
  {
    return o == Opcode::CallFar ||
           o == Opcode::CallNear ||
           o == Opcode::CallPtr ||
           o == Opcode::CallArgs;
  }

  constexpr uint16_t operation_size(Opcode o)
  {
    if(o == Opcode::Load8 || o == Opcode::Store8)
      return 1;
    if(o == Opcode::Load16 || o == Opcode::Store16)
      return 2;
    if(o == Opcode::Load32 || o == Opcode::Store32)
      return 4;
    panic("invalid");
    return 0;
  }

  constexpr bool has_no_result(Opcode o)
  {
    return o == Opcode::Store8 ||
           o == Opcode::Store16 ||
           o == Opcode::WriteVar8 ||
           o == Opcode::WriteVar16 ||
           o == Opcode::WriteVar32 ||
           o == Opcode::RetFar ||
           o == Opcode::RetNear ||
           o == Opcode::Jmp ||
           o == Opcode::Jne ||
           o == Opcode::JmpTbl;
  }

  constexpr bool maybe_unused(Opcode o)
  {
    return o == Opcode::Nop ||
           o == Opcode::Pin ||
           has_no_result(o) ||
           is_call(o) ||
           o == Opcode::Int;
  }

  constexpr bool has_side_effects(Opcode o)
  {
    return o == Opcode::Pin ||
           has_no_result(o) ||
           is_call(o) ||
           o == Opcode::AssertEven ||
           o == Opcode::AssertPos;
  }
}
