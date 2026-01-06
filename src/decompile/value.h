#pragma once

#include "symbols.h"

#include <cstdint>

enum {
  VALUE_TYPE_NONE = 0,
  VALUE_TYPE_SYM,
  VALUE_TYPE_MEM,
  VALUE_TYPE_IMM,
};

struct value_sym_t
{
  symref_t ref;
};

struct value_mem_t
{
  // TODO: Remove 8086-isms and dis86-isms
  int        sz; // SIZE_*
  symref_t   sreg;
  symref_t   reg1;
  symref_t   reg2;
  uint16_t        off;
};

struct value_imm_t
{
  // TODO: Remove 8086-isms and dis86-isms
  int sz; // SIZE_*
  uint16_t value;
};

struct value_t
{
  int type;
  union {
    value_sym_t sym[1];
    value_mem_t mem[1];
    value_imm_t imm[1];
  } u;
};

value_t value_from_operand(operand_t *o, symbols_t *symbols);
value_t value_from_symref(symref_t ref);
value_t value_from_imm(uint16_t imm);
bool    value_matches(value_t *a, value_t *b);

#define VALUE_NONE ({ \
  value_t v = {}; \
  v.type = VALUE_TYPE_NONE; \
  v; })

#define VALUE_IMM(_val) ({\
  value_t v = {};\
  v.type = VALUE_TYPE_IMM;\
  v.u.imm->sz = SIZE_16; \
  v.u.imm->value = _val;     \
  v; })

#define VALUE_IS_NONE(v) ((v).type == VALUE_TYPE_NONE)
