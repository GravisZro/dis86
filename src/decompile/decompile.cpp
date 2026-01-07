#include "decompile_private.h"

#include <format>

#define DEBUG_REPORT_SYMBOLS 0


static const char *n_bytes_as_type(uint16_t n_bytes)
{
  switch (n_bytes) {
    case 1: return "uint8_t";
    case 2: return "uint16_t";
    case 4: return "uint32_t";
    default: FAIL("Unknown size type | n_bytes: %u", n_bytes);
  }
}

typedef struct decompiler decompiler_t;
struct decompiler
{
  dis86_t *                  dis;
  dis86_decompile_config_t * cfg;
  dis86_decompile_config_t * default_cfg;
  const char *               func_name;
  uint16_t                        seg;
  dis86_instr_t *            ins;
  size_t                     n_ins;

  symbols_t * symbols;
  labels_t    labels[1];

  meh_t *meh;
};

static decompiler_t * decompiler_new( dis86_t *                  dis,
                                      dis86_decompile_config_t * opt_cfg,
                                      const char *               func_name,
                                      uint16_t                        seg,
                                      dis86_instr_t *            ins_arr,
                                      size_t                     n_ins )

{
  decompiler_t *d = (decompiler_t*)calloc(1, sizeof(decompiler_t));
  d->dis = dis;
  d->cfg = opt_cfg;
  if (!d->cfg) {
    d->default_cfg = config_default_new();
    d->cfg = d->default_cfg;
  }
  d->func_name = func_name;
  d->seg       = seg;
  d->ins       = ins_arr;
  d->n_ins     = n_ins;

  d->symbols = symbols_new();
  d->meh = nullptr;
  return d;
}

static void decompiler_delete(decompiler_t *d)
{
  if (d->meh) meh_delete(d->meh);
  if (d->default_cfg) config_delete(d->default_cfg);
  symbols_delete(d->symbols);
  free(d);
}

static void dump_symtab(symtab_t *symtab)
{
  symtab_iter_t it[1];
  symtab_iter_begin(it, symtab);
  while (1) {
    sym_t *var = symtab_iter_next(it);
    if (!var) break;

    const char *size;
    if (var->len <= 4) {
      size = n_bytes_as_type(var->len);
    } else {
      size = "UNKNOWN";
    }
    LOG_INFO("  %-30s | %04x | %6u | %s", sym_name(var).c_str(), (uint16_t)var->off, var->len, size);
  }
}

static void decompiler_initial_analysis(decompiler_t *d)
{
  // Pass to find all labels
  find_labels(d->labels, d->ins, d->n_ins);

  // Populate registers
  for (int reg_id = 1; reg_id < _REG_LAST; reg_id++) {
    sym_t deduced_sym[1];
    sym_deduce_reg(deduced_sym, reg_id);
    if (deduced_sym->len != 2) continue; // skip the small overlap regs
    symbols_insert_deduced(d->symbols, deduced_sym);
  }

  // Load all global symbols from config into the symtab
  for (size_t i = 0; i < d->cfg->global_len; i++) {
    config_global_t *g = &d->cfg->global_arr[i];

    type_t type[1];
    if (!type_parse(type, g->type)) {
      LOG_WARN("For global '%s', failed to parse type '%s' ... skipping", g->name, g->type);
      continue;
    }

    symbols_add_global(d->symbols, g->name, g->offset, type_size(type));
  }

  // Pass to locate all symbols
  for (size_t i = 0; i < d->n_ins; i++) {
    dis86_instr_t *ins = &d->ins[i];

    for (size_t j = 0; j < ARRAY_SIZE(ins->operand); j++) {
      operand_t *o = &ins->operand[j];
      if (o->type != OPERAND_TYPE_MEM) continue;

      sym_t deduced_sym[1];
      if (!sym_deduce(deduced_sym, &o->u.mem)) continue;

      if (!symbols_insert_deduced(d->symbols, deduced_sym)) {
        std::string name = sym_name(deduced_sym);
        LOG_WARN("Unknown global | name: %s  off: 0x%04x  size: %u", name.c_str(), (uint16_t)deduced_sym->off, deduced_sym->len);
      }
    }
  }

  // Pass to convert to expression structures
  d->meh = meh_new(d->cfg, d->symbols, d->seg, d->ins, d->n_ins);

  // Report the symbols
  if (DEBUG_REPORT_SYMBOLS) {
    LOG_INFO("Registers:");
    dump_symtab(d->symbols->registers);
    LOG_INFO("Globals:");
    dump_symtab(d->symbols->globals);
    LOG_INFO("Params:");
    dump_symtab(d->symbols->params);
    LOG_INFO("Locals:");
    dump_symtab(d->symbols->locals);
  }
}

static void decompiler_emit_preamble(decompiler_t *d, std::string& s)
{
  symtab_iter_t it[1];

  // Emit params
  symtab_iter_begin(it, d->symbols->params);
  while (1) {
    sym_t *var = symtab_iter_next(it);
    if (!var) break;

    std::string name = sym_name(var);
    s += std::format<"#define %s ARG_%zu(0x%x)\n">(name, 8*sym_size_bytes(var), var->off);
  }

  // Emit locals
  symtab_iter_begin(it, d->symbols->locals);
  while (1) {
    sym_t *var = symtab_iter_next(it);
    if (!var) break;

    std::string name = sym_name(var);
    s += std::format<"#define %s LOCAL_%zu(0x%x)\n">(name, 8*sym_size_bytes(var), -var->off);
  }

  s += std::format<"void %s(void)\n">(d->func_name);
  s += "{\n";
}

static void decompiler_emit_postamble(decompiler_t *d, std::string& s)
{
  symtab_iter_t it[1];

  s += "}\n";

  // Cleanup params
  symtab_iter_begin(it, d->symbols->params);
  sym_t* var = nullptr;
  while (var = symtab_iter_next(it), var != nullptr)
    s += std::format<"#undef %s\n">(sym_name(var));

  // Cleanup locals
  symtab_iter_begin(it, d->symbols->locals);
  while (var = symtab_iter_next(it), var != nullptr)
    s += std::format<"#undef %s\n">(sym_name(var));
}

std::string short_name(const std::string& name, size_t off, size_t n_bytes)
{
  std::string tmp;
  if (name == "AX" ||
      name == "BX" ||
      name == "CX" ||
      name == "DX")
  {
    tmp.push_back(name.front());
    assert(n_bytes == 1 && (off == 0 || off == 1));
    tmp.push_back(!off ? 'L' : 'H');
  }
  return tmp;
}

static std::string symref_lvalue_str(symref_t ref, const std::string& name)
{
  assert(ref.symbol);
  std::string s;

  if (ref.off == 0 && ref.len == ref.symbol->len)
    s = name;
  else if(s = short_name(name, ref.off, ref.len); s.empty())
    s = std::format<"*(%s*)((uint8_t*)&%s + %u)">(n_bytes_as_type(ref.len), name, ref.off);
  return s;
}

static std::string symref_rvalue_str(symref_t ref, const std::string& name)
{
  std::string s;
  assert(ref.symbol);
  if (ref.off == 0)
  {
    if (ref.len == ref.symbol->len)
      s = name;
    else
      s = std::format<"(%s)%s">(n_bytes_as_type(ref.len), name);
  }
  else if(s = short_name(name, ref.off, ref.len); s.empty())
    s = std::format<"(%s)(%s>>%u)">(n_bytes_as_type(ref.len), name, (8 * ref.off));
  return s;
}

static std::string value_str(value_t *v, bool as_lvalue)
{
  std::string s;

  switch (v->type) {
    case VALUE_TYPE_SYM: {
      if (as_lvalue) {
        s += symref_lvalue_str(v->u.sym->ref, sym_name(v->u.sym->ref.symbol));
      } else {
        s += symref_rvalue_str(v->u.sym->ref, sym_name(v->u.sym->ref.symbol));
      }
    } break;
    case VALUE_TYPE_MEM: {
      value_mem_t *m = v->u.mem;
      switch (m->sz) {
        case SIZE_8:  s += "*PTR_8("; break;
        case SIZE_16: s += "*PTR_16("; break;
        case SIZE_32: s += "*PTR_32("; break;
      }
      s += sym_name(m->sreg.symbol) + ", ";
      // FIXME: THIS IS ALL BROKEN BECAUSE IT ASSUMES THE SYMREF ARE NEVER PARTIAL REFS
      if (!m->reg1.symbol && !m->reg2.symbol)
      {
        if (m->off)
          s += std::format<"0x%x">(m->off);
      }
      else
      {
        if (m->reg1.symbol)
          s += sym_name(m->reg1.symbol);
        if (m->reg2.symbol)
          s += "+" + sym_name(m->reg2.symbol);
        if (m->off) {
          int16_t disp = (int16_t)m->off;
          /* if (disp >= 0) str_fmt(s, "+0x%x", (uint16_t)disp); */
          /* else           str_fmt(s, "-0x%x", (uint16_t)-disp); */
          s += std::format<"+0x%x">((uint16_t)disp);
        }
      }
      s += ")";
    } break;
    case VALUE_TYPE_IMM: {
      uint16_t val = v->u.imm->value;
      if (val == 0)
        s += "0";
      else
        s += std::format<"0x%x">(val);
    } break;
    default: FAIL("Unknown value type: %d\n", v->type);
  }
  return s;
}

static void decompiler_emit_expr(decompiler_t *d, std::string& ret_s, expr_t *expr)
{
  std::string s;
  switch (expr->kind) {
    case EXPR_KIND_NONE: {
      return;
    } break;
    case EXPR_KIND_UNKNOWN: {
      s += "UNKNOWN();";
    } break;
    case EXPR_KIND_OPERATOR1: {
      expr_operator1_t *k = expr->k.operator1;
      assert(!k->op.sign); // not sure what this would mean...
      s += value_str(&k->dest, true);
      s += std::format<" %s ;">(k->op);
    } break;
    case EXPR_KIND_OPERATOR2: {
      expr_operator2_t *k = expr->k.operator2;
      if (k->op.sign)
        s += "(int16_t)";
      s += value_str(&k->dest, true);
      s += std::format<" %s ">(k->op);
      if (k->op.sign)
        s += "(int16_t)";
      s += value_str(&k->src, false) + ";";
    } break;
    case EXPR_KIND_OPERATOR3: {
      expr_operator3_t *k = expr->k.operator3;
      s += value_str(&k->dest, true) + " = ";
      if (k->op.sign)
        s += "(int16_t)";
      s += value_str(&k->left, false);
      s += std::format<" %s ">(k->op);

      if (k->op.sign)
        s += "(int16_t)";
      s += value_str(&k->right, false) + ";";
    } break;
    case EXPR_KIND_ABSTRACT: {
      expr_abstract_t *k = expr->k.abstract;
      if (!VALUE_IS_NONE(k->ret)) {
        s += value_str(&k->ret, true) + " = ";
      }
      s += k->func_name;
      s += "(";
      for (size_t i = 0; i < k->n_args; i++) {
        if (i)
          s += ", ";
        s += value_str(&k->args[i], false);
      }
      s += ");";
    } break;

    case EXPR_KIND_BRANCH_COND: {
      expr_branch_cond_t *k = expr->k.branch_cond;
      s += "if (";
      if (k->op.sign)
        s += "(int16_t)";
      s += value_str(&k->left, false);
      s += std::format<" %s ">(k->op);
      if (k->op.sign)
        s += "(int16_t)";
      s += std::format<") goto label_%08x;">(k->target);
    } break;
    case EXPR_KIND_BRANCH_FLAGS: {
      expr_branch_flags_t *k = expr->k.branch_flags;
      s += std::format<"if (%s(">(k->op);
      s += value_str(&k->flags, false);
      s += std::format<")) goto label_%08x;">(k->target);
    } break;
    case EXPR_KIND_BRANCH: {
      expr_branch_t *k = expr->k.branch;
      s += std::format<"goto label_%08x;">(k->target);
    } break;
    case EXPR_KIND_CALL: {
      expr_call_t *k = expr->k.call;
      if (k->func) {
        s += std::format<"CALL_FUNC(%s);">(k->func->name);
      } else {
        switch (k->addr.type) {
          case addr_type_e::ADDR_TYPE_FAR: {
              s += std::format<"CALL_FAR(0x%04x, 0x%04x);">(k->addr.u.far.seg, k->addr.u.far.off);
          } break;
          case addr_type_e::ADDR_TYPE_NEAR: {
              s += std::format<"CALL_NEAR(0x%04x);">(k->addr.u.near);
          } break;
          default: {
              FAIL("Unknonw address type: %d", int(k->addr.type));
          } break;
        }
      }
      if (k->remapped)
        s += " /* remapped */";
    } break;
    case EXPR_KIND_CALL_WITH_ARGS: {
      expr_call_with_args_t *k = expr->k.call_with_args;
      s += std::format<"%s(m">(k->func->name);
      for (size_t i = 0; i < (size_t)k->func->args; i++) {
        s += ", " + value_str(&k->args[i], false);
      }
      s += ");";
      if (k->remapped)
        s += " /* remapped */";
    } break;
    default: {
      s += "UNIMPL();";
    } break;
  }

  for (size_t i = 0; i < expr->n_ins; i++)
  {
    std::string as = dis86_print_intel_syntax(d->dis, &expr->ins[i], false);
    std::string cs = i+1 == expr->n_ins ? s : "";
    ret_s += std::format<"  %-50s // %s\n">(cs, as);
  }
}

std::string dis86_decompile( dis86_t *                  dis,
                       dis86_decompile_config_t * opt_cfg,
                       const char *               func_name,
                       uint16_t                        seg,
                       dis86_instr_t *            ins_arr,
                       size_t                     n_ins )
{
  std::string s;

  decompiler_t *d = decompiler_new(dis, opt_cfg, func_name, seg, ins_arr, n_ins);
  decompiler_initial_analysis(d);
  decompiler_emit_preamble(d, s);

  for (size_t i = 0; i < d->meh->expr_len; i++) {
    expr_t *expr = &d->meh->expr_arr[i];
    if (expr->n_ins > 0 && is_label(d->labels, (uint32_t)expr->ins->addr)) {
      s += std::format<"\n label_%08x:\n">((uint32_t)expr->ins->addr);
    }
    decompiler_emit_expr(d, s, expr);
  }

  decompiler_emit_postamble(d, s);
  return s;
}
