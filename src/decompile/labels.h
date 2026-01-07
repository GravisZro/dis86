#include <cstdint>
#include <unistd.h>

#include "instr.h"

#define MAX_LABELS 256

typedef struct labels labels_t;
struct labels
{
  uint32_t addr[MAX_LABELS];
  size_t n_addr;
};

// FIXME: O(n) search
static bool is_label(labels_t *labels, uint32_t addr)
{
  for (size_t i = 0; i < labels->n_addr; i++) {
    if (labels->addr[i] == addr) return true;
  }
  return false;
}

static uint32_t branch_destination(dis86_instr_t *ins)
{
  int16_t rel = 0;
  switch (ins->opcode) {
    case operation_e::JO:  rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JNO: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JB:  rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JAE: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JE:  rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JNE: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JBE: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JA:  rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JS:  rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JNS: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JP:  rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JNP: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JL:  rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JGE: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JLE: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JG:  rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::JMP: rel = (int16_t)ins->operand[0].u.rel.val; break;
    case operation_e::LOOP:rel = (int16_t)ins->operand[1].u.rel.val; break;
    default: return 0;
  }

  uint16_t effective = ins->addr + ins->n_bytes + rel;
  return effective;
}

static void find_labels(labels_t *labels, dis86_instr_t *ins_arr, size_t n_ins)
{
  labels->n_addr = 0;

  for (size_t i = 0; i < n_ins; i++) {
    dis86_instr_t *ins = &ins_arr[i];
    uint16_t dst = branch_destination(ins);
    if (!dst) continue;

    assert(labels->n_addr < ARRAY_SIZE(labels->addr));
    labels->addr[labels->n_addr++] = dst;
  }
}
