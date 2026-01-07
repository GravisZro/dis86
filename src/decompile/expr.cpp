#include "decompile_private.h"

static size_t _impl_operator1(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins,
                              const char *_oper, int _sign)
{
  assert(ins->operand[0].type != OPERAND_TYPE_NONE);

  expr->kind = EXPR_KIND_OPERATOR1;
  expr_operator1_t *k = expr->k.operator1;
  k->op.oper = _oper;
  k->op.sign = _sign;
  k->dest     = value_from_operand(&ins->operand[0], symbols);

  return 1;
}

static size_t _impl_operator2(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins,
                              const char *_oper, int _sign)
{
  assert(ins->operand[0].type != OPERAND_TYPE_NONE);
  assert(ins->operand[1].type != OPERAND_TYPE_NONE);

  expr->kind = EXPR_KIND_OPERATOR2;
  expr_operator2_t *k = expr->k.operator2;
  k->op.oper = _oper;
  k->op.sign = _sign;
  k->dest     = value_from_operand(&ins->operand[0], symbols);
  k->src      = value_from_operand(&ins->operand[1], symbols);

  return 1;
}

static size_t _impl_operator3(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins,
                              const char *_oper, int _sign)
{
  assert(ins->operand[0].type != OPERAND_TYPE_NONE);
  assert(ins->operand[1].type != OPERAND_TYPE_NONE);
  assert(ins->operand[2].type != OPERAND_TYPE_NONE);

  expr->kind = EXPR_KIND_OPERATOR3;
  expr_operator3_t *k = expr->k.operator3;
  k->op.oper = _oper;
  k->op.sign = _sign;
  k->dest     = value_from_operand(&ins->operand[0], symbols);
  k->left     = value_from_operand(&ins->operand[1], symbols);
  k->right    = value_from_operand(&ins->operand[2], symbols);

  return 1;
}

static size_t _impl_abstract(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins,
                             const char *_name)
{
  expr->kind = EXPR_KIND_ABSTRACT;
  expr_abstract_t *k = expr->k.abstract;
  k->func_name = _name;
  k->ret = VALUE_NONE;
  k->n_args = 0;

  assert(ARRAY_SIZE(k->args) <= ARRAY_SIZE(ins->operand));
  for (size_t i = 0; i < ARRAY_SIZE(ins->operand); i++) {
    operand_t *o = &ins->operand[i];
    if (o->type == OPERAND_TYPE_NONE) break;
    k->args[k->n_args++] = value_from_operand(o, symbols);
  }

  return 1;
}

static size_t _impl_abstract_ret(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins,
                                 const char *_name)
{
  assert(ins->operand[0].type != OPERAND_TYPE_NONE);

  expr->kind = EXPR_KIND_ABSTRACT;
  expr_abstract_t *k = expr->k.abstract;
  k->func_name = _name;
  k->ret = value_from_operand(&ins->operand[0], symbols);
  k->n_args = 0;

  assert(ARRAY_SIZE(k->args) <= ARRAY_SIZE(ins->operand));
  for (size_t i = 1; i < ARRAY_SIZE(ins->operand); i++) {
    operand_t *o = &ins->operand[i];
    if (o->type == OPERAND_TYPE_NONE) break;
    k->args[k->n_args++] = value_from_operand(o, symbols);
  }

  return 1;
}

static size_t _impl_abstract_flags(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins,
                                   const char *_name)
{
  expr->kind = EXPR_KIND_ABSTRACT;
  expr_abstract_t *k = expr->k.abstract;
  k->func_name = _name;
  k->ret = value_from_symref(symbols_find_reg(symbols, REG_FLAGS));
  k->n_args = 0;

  assert(ARRAY_SIZE(k->args) <= ARRAY_SIZE(ins->operand));
  for (size_t i = 0; i < ARRAY_SIZE(ins->operand); i++) {
    operand_t *o = &ins->operand[i];
    if (o->type == OPERAND_TYPE_NONE) break;
    k->args[k->n_args++] = value_from_operand(o, symbols);
  }

  return 1;
}

static size_t _impl_abstract_jump(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins,
                                  const char *_operation)
{
  assert(ins->operand[0].type == OPERAND_TYPE_REL);
  assert(ins->operand[1].type == OPERAND_TYPE_NONE);

  expr->kind = EXPR_KIND_BRANCH_FLAGS;
  expr_branch_flags_t *k = expr->k.branch_flags;
  k->op     = _operation;
  k->flags  = value_from_symref(symbols_find_reg(symbols, REG_FLAGS));
  k->target = ins->addr + ins->n_bytes + (int16_t)ins->operand[0].u.rel.val;

  return 1;
}

/* static size_t _impl_abstract_loop(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins) */
/* { */
/*   STOPPED_HERE; */
/* } */

static size_t _impl_call_far(expr_t *expr, dis86_decompile_config_t *cfg, symbols_t *symbols, dis86_instr_t *ins)
{
  // FIXME BROKEN!!!
  if (ins->operand[0].type != OPERAND_TYPE_FAR) {
    expr->kind = EXPR_KIND_UNKNOWN;
    return 1;
  }

  operand_far_t *far = &ins->operand[0].u.far;
  segoff_t addr = {far->seg, far->off};
  bool remapped = config_seg_remap(cfg, &addr.seg);
  config_func_t *func = config_func_lookup(cfg, addr);

  expr->kind = EXPR_KIND_CALL;
  expr_call_t *k = expr->k.call;
  k->addr.type  = addr_type_e::ADDR_TYPE_FAR;
  k->addr.u.far = addr;
  k->remapped   = remapped;
  k->func       = func;

  return 1;
}

static size_t _impl_call_near(uint16_t seg, expr_t *expr,
                              dis86_decompile_config_t *cfg, symbols_t *symbols, dis86_instr_t *ins)
{
  if (ins->operand[0].type != OPERAND_TYPE_REL) {
    expr->kind = EXPR_KIND_UNKNOWN;
    return 1;
  }

  size_t effective = (uint16_t)(ins->addr + ins->n_bytes + ins->operand[0].u.rel.val);
  //printf("effective: 0x%x | seg: 0x%x\n", (uint32_t)effective, seg);
  assert(16*(size_t)seg <= effective && effective < 16*(size_t)seg + (1<<16));
  uint16_t off = effective - 16*(size_t)seg;

  segoff_t addr = {seg, off};
  config_func_t *func = config_func_lookup(cfg, addr);

  expr->kind = EXPR_KIND_CALL;
  expr_call_t *k = expr->k.call;
  k->addr.type   = addr_type_e::ADDR_TYPE_NEAR;
  k->addr.u.near = effective;
  k->remapped    = false;
  k->func        = func;

  return 1;
}

static size_t _impl_load_effective_addr(expr_t *expr, symbols_t *symbols, dis86_instr_t *ins)
{
  assert(ins->operand[0].type != OPERAND_TYPE_NONE);

  assert(ins->operand[1].type == OPERAND_TYPE_MEM);
  operand_mem_t *mem = &ins->operand[1].u.mem;
  assert(mem->sz == SIZE_16);
  assert(mem->reg1);
  assert(!mem->reg2);
  assert(mem->off);

  expr->kind = EXPR_KIND_OPERATOR3;
  expr_operator3_t *k = expr->k.operator3;
  k->op.oper = "-";
  k->op.sign = 0;
  k->dest = value_from_operand(&ins->operand[0], symbols);
  k->left = value_from_symref(symbols_find_reg(symbols, mem->reg1));
  k->right = value_from_imm(-(int16_t)mem->off);

  return 1;
}

#define OPERATOR1(_oper, _sign)  _impl_operator1(expr, symbols, ins, _oper, _sign)
#define OPERATOR2(_oper, _sign)  _impl_operator2(expr, symbols, ins, _oper, _sign)
#define OPERATOR3(_oper, _sign)  _impl_operator3(expr, symbols, ins, _oper, _sign)
#define ABSTRACT(_name)          _impl_abstract(expr, symbols, ins, _name)
#define ABSTRACT_RET(_name)      _impl_abstract_ret(expr, symbols, ins, _name)
#define ABSTRACT_FLAGS(_name)    _impl_abstract_flags(expr, symbols, ins, _name)
#define ABSTRACT_JUMP(_op)       _impl_abstract_jump(expr, symbols, ins, _op)
//#define ABSTRACT_LOOP()          _impl_abstract_loop(expr, symbols, ins)
#define CALL_FAR()               _impl_call_far(expr, cfg, symbols, ins)
#define CALL_NEAR()              _impl_call_near(seg, expr, cfg, symbols, ins)
#define LOAD_EFFECTIVE_ADDR()    _impl_load_effective_addr(expr, symbols, ins)

static size_t extract_expr(uint16_t seg, expr_t *expr, dis86_decompile_config_t *cfg, symbols_t *symbols,
                           dis86_instr_t *ins, size_t n_ins)
{
  switch (ins->opcode)
  {
    case operation_e::AAA:    break;
    case operation_e::AAS:    break;
    case operation_e::ADC:    break;
    case operation_e::ADD:    return OPERATOR2("+=", 0);
    case operation_e::AND:    return OPERATOR2("&=", 0);
    case operation_e::CALL:   return CALL_NEAR();
    case operation_e::CALLF:  return CALL_FAR();
    case operation_e::CBW:    break;
    case operation_e::CLC:    break;
    case operation_e::CLD:    break;
    case operation_e::CLI:    break;
    case operation_e::CMC:    break;
    case operation_e::CMP:    return ABSTRACT_FLAGS("CMP");
    case operation_e::CMPS:   break;
    case operation_e::CWD:    break;
    case operation_e::DAA:    break;
    case operation_e::DAS:    break;
    case operation_e::DEC:    return OPERATOR1("-= 1", 0);
    case operation_e::DIV:    break;
    case operation_e::ENTER:  break;
    case operation_e::HLT:    break;
    case operation_e::IMUL:   return OPERATOR3("*", 1);
    case operation_e::IN:     break;
    case operation_e::INC:    return OPERATOR1("+= 1", 0);
    case operation_e::INS:    break;
    case operation_e::INT:    break;
    case operation_e::INTO:   break;
    case operation_e::INVAL:  break;
    case operation_e::IRET:   break;
    case operation_e::JA:     return ABSTRACT_JUMP("JA");
    case operation_e::JAE:    return ABSTRACT_JUMP("JAE");
    case operation_e::JB:     return ABSTRACT_JUMP("JB");
    case operation_e::JBE:    return ABSTRACT_JUMP("JBE");
    case operation_e::JCXZ:   break;
    case operation_e::JE:     return ABSTRACT_JUMP("JE");
    case operation_e::JG:     return ABSTRACT_JUMP("JG");
    case operation_e::JGE:    return ABSTRACT_JUMP("JGE");
    case operation_e::JL:     return ABSTRACT_JUMP("JL");
    case operation_e::JLE:    return ABSTRACT_JUMP("JLE");
    case operation_e::JMP: {
      expr->kind = EXPR_KIND_BRANCH;
      expr_branch_t *k = expr->k.branch;
      k->target = branch_destination(ins);
      return 1;
    } break;
    case operation_e::JMPF:   break;
    case operation_e::JNE:    return ABSTRACT_JUMP("JNE");
    case operation_e::JNO:    break;
    case operation_e::JNP:    break;
    case operation_e::JNS:    break;
    case operation_e::JO:     break;
    case operation_e::JP:     break;
    case operation_e::JS:     break;
    case operation_e::LAHF:   break;
    case operation_e::LDS:    return ABSTRACT("LOAD_SEG_OFF");
    case operation_e::LEA:    return LOAD_EFFECTIVE_ADDR();
    case operation_e::LEAVE:  return ABSTRACT("LEAVE");
      //case operation_e::LEAVE:  LITERAL("SP = BP; BP = POP();
    case operation_e::LES:    return ABSTRACT("LOAD_SEG_OFF");
    case operation_e::LODS:   break;
    case operation_e::LOOP:   break; //return ABSTRACT_LOOP();
    case operation_e::LOOPE:  break;
    case operation_e::LOOPNE: break;
    case operation_e::MOV:    return OPERATOR2("=", 0);
    case operation_e::MOVS:   break;
    case operation_e::MUL:    break;
    case operation_e::NEG:    break;
    case operation_e::NOP:    break;
    case operation_e::NOT:    break;
    case operation_e::OR:     return OPERATOR2("|=", 0);
    case operation_e::OUT:    break;
    case operation_e::OUTS:   break;
    case operation_e::POP:    return ABSTRACT_RET("POP");
    case operation_e::POPA:   break;
    case operation_e::POPF:   break;
    case operation_e::PUSH:   return ABSTRACT("PUSH");
    case operation_e::PUSHA:  break;
    case operation_e::PUSHF:  break;
    case operation_e::RCL:    break;
    case operation_e::RCR:    break;
    case operation_e::RET:    return ABSTRACT("RETURN_NEAR");
    case operation_e::RETF:   return ABSTRACT("RETURN_FAR");
    case operation_e::ROL:    break;
    case operation_e::ROR:    break;
    case operation_e::SAHF:   break;
    case operation_e::SAR:    break;
    case operation_e::SBB:    break;
    case operation_e::SCAS:   break;
    case operation_e::SHL:    return OPERATOR2("<<=", 0);
    case operation_e::SHR:    return OPERATOR2(">>=", 0);
    case operation_e::STC:    break;
    case operation_e::STD:    break;
    case operation_e::STI:    break;
    case operation_e::STOS:   break;
    case operation_e::SUB:    return OPERATOR2("-=", 0);
    case operation_e::TEST:   return ABSTRACT_FLAGS("TEST");
    case operation_e::XCHG:   break;
    case operation_e::XLAT:   break;
    case operation_e::XOR:    return OPERATOR2("^=", 0);
    default: FAIL("Unknown Instruction: %d", ins->opcode);
  }

  // If we reach this point, the instruction wasn't mapped to any expression: UNKNOWN
  expr->kind = EXPR_KIND_UNKNOWN;
  return 1;
}

value_t expr_destination(expr_t *expr)
{
  switch (expr->kind) {
    case EXPR_KIND_UNKNOWN:       FAIL("EXPR_KIND_UNKNOWN UNSUPPORTED");
    case EXPR_KIND_NONE:          return VALUE_NONE;
    case EXPR_KIND_OPERATOR1:     return expr->k.operator1->dest;
    case EXPR_KIND_OPERATOR2:     return expr->k.operator2->dest;
    case EXPR_KIND_OPERATOR3:     return expr->k.operator3->dest;
    case EXPR_KIND_ABSTRACT:      return expr->k.abstract->ret;
    case EXPR_KIND_BRANCH_COND:   return VALUE_NONE;
    case EXPR_KIND_BRANCH_FLAGS:  return expr->k.branch_flags->flags;
    case EXPR_KIND_BRANCH:        return VALUE_NONE;
    case EXPR_KIND_CALL:          return VALUE_NONE;  // ??? is this true ?? Should this be AX:DX ??
    default: FAIL("Unkown expression kind: %d", expr->kind);
  }
}

meh_t * meh_new(dis86_decompile_config_t *cfg, symbols_t *symbols, uint16_t seg, dis86_instr_t *ins, size_t n_ins)
{
  meh_t *m = (meh_t*)calloc(1, sizeof(meh_t));

  while (n_ins) {
    assert(m->expr_len < ARRAY_SIZE(m->expr_arr));

    expr_t *expr = &m->expr_arr[m->expr_len];
    size_t consumed = extract_expr(seg, expr, cfg, symbols, ins, n_ins);
    assert(consumed <= n_ins);
    expr->ins = ins;
    expr->n_ins = consumed;
    m->expr_len++;

    ins += consumed;
    n_ins -= consumed;
  }

  transform_pass_xor_rr(m);
  transform_pass_cmp_jmp(m);
  transform_pass_or_jmp(m);
  transform_pass_synthesize_calls(m);

  return m;
}

void meh_delete(meh_t *m)
{
  free(m);
}
