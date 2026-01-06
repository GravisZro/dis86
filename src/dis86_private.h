#ifndef DIS86_PRIVATE_H
#define DIS86_PRIVATE_H

#include "header.h"
#include "binary.h"
#include "instr.h"

enum {
  RESULT_SUCCESS = 0,
  RESULT_NEED_OPCODE2,
  RESULT_NOT_FOUND,
};

struct dis86_t
{
  binary_t b[1];
  dis86_instr_t ins[1];
};

#endif
