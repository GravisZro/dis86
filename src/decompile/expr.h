#pragma once
#include "dis86.h"
#include "config.h"
#include "value.h"

#include <ostream>

enum class addr_type_e
{
  ADDR_TYPE_FAR,
  ADDR_TYPE_NEAR,
};

struct addr_t
{
  addr_type_e type;
  union {
    segoff_t far;
    uint16_t      near;
  } u;
};

struct operator_t
{
  const char* oper;
  int          sign;
};




enum {
  EXPR_KIND_UNKNOWN,
  EXPR_KIND_NONE,
  EXPR_KIND_OPERATOR1,
  EXPR_KIND_OPERATOR2,
  EXPR_KIND_OPERATOR3,
  EXPR_KIND_ABSTRACT,
  EXPR_KIND_BRANCH_COND,
  EXPR_KIND_BRANCH_FLAGS,
  EXPR_KIND_BRANCH,
  EXPR_KIND_CALL,
  EXPR_KIND_CALL_WITH_ARGS,
};

struct expr_operator1_t
{
  operator_t   op;
  value_t      dest;
};

struct expr_operator2_t
{
  operator_t   op;
  value_t      dest;
  value_t      src;
};

struct expr_operator3_t
{
  operator_t   op;
  value_t      dest;
  value_t      left;
  value_t      right;
};

struct expr_abstract_t
{
  const char * func_name;
  value_t      ret;
  uint16_t          n_args;
  value_t      args[3];
};

struct expr_branch_cond_t
{
  operator_t   op;
  value_t      left;
  value_t      right;
  uint32_t          target;
};

struct expr_branch_flags_t
{
  const char * op; // FIXME
  value_t      flags;
  uint32_t          target;
};

struct expr_branch_t
{
  uint32_t target;
};

struct expr_call_t
{
  addr_t          addr;
  bool            remapped;
  config_func_t * func; // optional
};

#define MAX_ARGS 16
struct expr_call_with_args_t
{
  addr_t          addr;
  bool            remapped;
  config_func_t * func; // required
  value_t         args[MAX_ARGS];
};

struct expr_t
{
  int kind;
  union {
    expr_operator1_t      operator1[1];
    expr_operator2_t      operator2[1];
    expr_operator3_t      operator3[1];
    expr_abstract_t       abstract[1];
    expr_branch_cond_t    branch_cond[1];
    expr_branch_flags_t   branch_flags[1];
    expr_branch_t         branch[1];
    expr_call_t           call[1];
    expr_call_with_args_t call_with_args[1];
  } k;

  size_t          n_ins;
  dis86_instr_t * ins;
};

value_t expr_destination(expr_t *expr);

#define EXPR_NONE ({ \
  expr_t expr = {}; \
  expr.kind = EXPR_KIND_NONE; \
  expr; })

#define EXPR_MAX 4096
struct meh_t
{
  size_t expr_len;
  expr_t expr_arr[EXPR_MAX];
};

meh_t * meh_new(dis86_decompile_config_t *cfg, symbols_t *symbols, uint16_t seg, dis86_instr_t *ins, size_t n_ins);
void    meh_delete(meh_t *m);
