#pragma once
#include "header.h"

#include <array>

#define OPERAND_MAX 3

#define REGISTER_ARRAY(_)\
  /* Standard 16-bit registers */ \
  _( REG_AX,    16, "ax",    "AX"    )\
  _( REG_CX,    16, "cx",    "CX"    )\
  _( REG_DX,    16, "dx",    "DX"    )\
  _( REG_BX,    16, "bx",    "BX"    )\
  _( REG_SP,    16, "sp",    "SP"    )\
  _( REG_BP,    16, "bp",    "BP"    )\
  _( REG_SI,    16, "si",    "SI"    )\
  _( REG_DI,    16, "di",    "DI"    )\
  /* Standard 8-bit registers (may overlap with above) */\
  _( REG_AL,     8, "al",    "AL"    )\
  _( REG_CL,     8, "cl",    "CL"    )\
  _( REG_DL,     8, "dl",    "DL"    )\
  _( REG_BL,     8, "bl",    "BL"    )\
  _( REG_AH,     8, "ah",    "AH"    )\
  _( REG_CH,     8, "ch",    "CH"    )\
  _( REG_DH,     8, "dh",    "DH"    )\
  _( REG_BH,     8, "bh",    "BH"    )\
  /* Segment registers */\
  _( REG_ES,    16, "es",    "ES"    )\
  _( REG_CS,    16, "cs",    "CS"    )\
  _( REG_SS,    16, "ss",    "SS"    )\
  _( REG_DS,    16, "ds",    "DS"    )\
  /* Other registers */\
  _( REG_IP,    16, "ip",    "IP"    )\
  _( REG_FLAGS, 16, "flags", "FLAGS" )\

enum {
  REG_INVAL = 0,
#define ELT(r, _1, _2, _3) r,
  REGISTER_ARRAY(ELT)
#undef ELT
  _REG_LAST,
};

static inline const char *reg_name(int reg)
{
  static const char *arr[] = {
    nullptr,
#define ELT(_1, _2, s, _3) s,
  REGISTER_ARRAY(ELT)
#undef ELT
  };
  if ((size_t)reg >= ARRAY_SIZE(arr)) return nullptr;
  return arr[reg];
}

static inline const char *reg_name_upper(int reg)
{
  static const char *arr[] = {
    nullptr,
#define ELT(_1, _2, _3, s) s,
  REGISTER_ARRAY(ELT)
#undef ELT
  };
  if ((size_t)reg >= ARRAY_SIZE(arr)) return nullptr;
  return arr[reg];
}


enum class operand_e
{
  None,
  // Implied 16-bit register operands
  AX, CX, DX, BX,
  SP, BP, SI, DI,

  // Implied 8-bit register operands
  AL, CL, DL, BL,
  AH, CH, DH, BH,

  // Implied segment regsiter operands
  ES, CS, SS, DS,

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

enum class operation_e
{
   AAA=0,  AAS,  ADC,   ADD,  AND, CALL, CALLF,   CBW,
     CLC,  CLD,  CLI,   CMC,  CMP, CMPS,   CWD,   DAA,
     DAS,  DEC,  DIV, ENTER,  HLT, IMUL,    IN,   INC,
     INS,  INT, INTO, INVAL, IRET,   JA,   JAE,    JB,
     JBE, JCXZ,   JE,    JG,  JGE,   JL,   JLE,   JMP,
    JMPF,  JNE,  JNO,   JNP,  JNS,   JO,    JP,    JS,
    LAHF,  LDS,  LEA, LEAVE,  LES, LODS,  LOOP, LOOPE,
  LOOPNE,  MOV, MOVS,   MUL,  NEG,  NOP,   NOT,    OR,
     OUT, OUTS,  POP,  POPA, POPF, PUSH, PUSHA, PUSHF,
     RCL,  RCR,  RET,  RETF,  ROL,  ROR,  SAHF,   SAR,
     SBB, SCAS,  SHL,   SHR,  STC,  STD,   STI,  STOS,
     SUB, TEST, XCHG,  XLAT,  XOR,
};


enum {
  OPERAND_TYPE_NONE = 0,
  OPERAND_TYPE_REG,
  OPERAND_TYPE_MEM,
  OPERAND_TYPE_IMM,
  OPERAND_TYPE_REL,
  OPERAND_TYPE_FAR,
};

typedef struct operand_reg_t
{
  int id;
} operand_reg_t;

enum {
  SIZE_8,
  SIZE_16,
  SIZE_32,
};

struct operand_mem_t
{
  int sz;   // SIZE_
  int sreg; // always must be populated
  int reg1; // 0 if unused
  int reg2; // 0 if unused
  uint16_t off;  // 0 if unused
};

struct operand_imm_t
{
  int sz;
  uint16_t val;
};

struct operand_rel_t
{
  uint16_t val;
};

struct operand_far_t
{
  uint16_t seg;
  uint16_t off;
};

struct operand_t
{
  int type;
  union {
    operand_reg_t reg;
    operand_mem_t mem;
    operand_imm_t imm;
    operand_rel_t rel;
    operand_far_t far;
  } u;
};

enum {
  REP_NONE = 0,
  REP_NE,
  REP_E,
};

struct dis86_instr_t
{
  int       rep;
  operation_e opcode;
  operand_t operand[OPERAND_MAX];
  size_t    addr;
  size_t    n_bytes;
  int       intel_hidden;   /* bitmap of operands hidden in intel assembly */
};

struct instr_fmt_t
{
  operation_e op;             /* operation_e:: */
  uint8_t opcode1;        /* first byte: opcode */
  uint8_t opcode2;        /* 3-bit modrm reg field: sometimes used as level 2 opcode */
  std::array<operand_e, 3> operands;       /* operand:: */
  uint8_t intel_hidden;   /* bitmap of operands hidden in intel assembly */
};


extern std::array<instr_fmt_t, 367> instr_tbl;

extern const std::array<const char* const, 93> instr_op_mneumonic;
int instr_fmt_lookup(uint8_t opcode1, uint8_t opcode2, instr_fmt_t **fmt);
