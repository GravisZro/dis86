#pragma once

#include "common/result.h"

#include <array>
#include <string>
#include <optional>

namespace instr_fmt
{
  enum class operand_e : uint8_t
  {
    // Invalid
    NONE = 0,

    // Implied 16-bit register operands
    AX,  CX,  DX,  BX,
    SP,  BP,  SI,  DI,

    // Implied 8-bit register operands
    AL,  CL,  DL,  BL,
    AH,  CH,  DH,  BH,

    // Implied segment regsiter operands
    ES,  CS,  SS,  DS,

    // Implied others
    FLAGS,
    LIT1,
    LIT3,

    // Implied string operations operands
    SRC8,
    SRC16,
    DST8,
    DST16,

    // Explicit register operands
    R8,     // Register field from ModRM byte
    R16,    // Register field from ModRM byte
    SREG,   // Second register field from ModRM byte (interpreted as an SREG)

    // Explicit memory operands
    M8,     // Memory operand to address 8-bit from ModRM (no reg allowed)
    M16,    // Memory operand to address 16-bit from ModRM (no reg allowed)
    M32,    // Memory operand to address 32-bit from ModRM (no reg allowed)

    // Explicit register or memory operands (modrm)
    RM8,    // Either Register of memory operand, always 8-bit
    RM16,   // Either Register of memory operand, always 16-bit

    // Explicit immediate data
    IMM8,     // Immediate value, sized 8-bits
    IMM8_EXT, // Immediate value, sized 8-bits, sign-extended to 16-bits
    IMM16,    // Immediate value, sized 16-bits

    // Explicit far32 jump immediate
    FAR32,  // Immediate value, sized 32-bits

    // Explicit 16-bit immediate used as a memory offset into DS
    MOFF8,  // 16-bit imm loading 8-bit value
    MOFF16, // 16-bit imm loading 16-bit value

    // Explicit relative offsets (branching / calls)
    REL8,   // Sign-extended to 16-bit and added to address after fetch
    REL16,  // Added to address after fetch
  };

  enum class operation_e : uint8_t
  {
    AAA = 0,AAS,    ADC,        ADD,
    AND,    CALL,   CALLF,      CBW,
    CLC,    CLD,    CLI,        CMC,
    CMP,    CMPS,   CWD,        DAA,
    DAS,    DEC,    DIV,        ENTER,
    HLT,    IMUL,   IMUL_TRUNC, IN,
    INC,    INS,    INT,        INTO,
    INVAL,  IRET,   JA,         JAE,
    JB,     JBE,    JCXZ,       JE,
    JG,     JGE,    JL,         JLE,
    JMP,    JMPF,   JNE,        JNO,
    JNP,    JNS,    JO,         JP,
    JS,     LAHF,   LDS,        LEA,
    LEAVE,  LES,    LODS,       LOOP,
    LOOPE,  LOOPNE, MOV,        MOVS,
    MUL,    NEG,    NOP,        NOT,
    OR,     OUT,    OUTS,       POP,
    POPA,   POPF,   PUSH,       PUSHA,
    PUSHF,  RCL,    RCR,        RET,
    RETF,   ROL,    ROR,        SAHF,
    SAR,    SBB,    SCAS,       SETO,
    SETNO,  SETA,   SETAE,      SETB,
    SETBE,  SETE,   SETG,       SETGE,
    SETL,   SETLE,  SETP,       SETS,
    SETNE,  SETNP,  SETNS,      SHL,
    SHR,    STC,    STD,        STI,
    STOS,   SUB,    TEST,       XCHG,
    XLAT,   XOR,
  };


  enum class Error
  {
    NotFound,
    NeedOpcode2,
    NeedOpcode2Ext0F,
  };

  struct instruction_format_t
  {
    operation_e op;
    uint8_t opcode1;        /* first byte: opcode */
    uint8_t opcode2;        /* 3-bit modrm reg field: sometimes used as level 2 opcode */
    std::array<operand_e, 3> operands;
    uint8_t hidden;

    bool requires_modrm(void) const
    {
      for(const auto& o : operands)
        switch(o)
        {
          case operand_e::R8:
          case operand_e::R16:
          case operand_e::SREG:
          case operand_e::M8:
          case operand_e::M16:
          case operand_e::M32:
          case operand_e::RM8:
          case operand_e::RM16:
            return true;
          default:
            break;
        }
      return false;
    }
  };

  Result<instruction_format_t, Error> lookup(uint8_t opcode1, std::optional<uint8_t> opcode2);
  extern std::array<instruction_format_t, 367> instr_tbl;
  extern const std::array<std::string, 110> instr_op_mneumonic;
}
