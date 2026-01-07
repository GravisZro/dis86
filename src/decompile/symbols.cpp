#include "decompile_private.h"
#include <stdalign.h>

#include <format>
#include <string>

static uint16_t size_in_bytes(int sz)
{
  switch (sz) {
    case SIZE_8:  return 1;
    case SIZE_16: return 2;
    case SIZE_32: return 4;
    default: FAIL("Unknown size type");
  }
}

bool sym_deduce(sym_t *s, operand_mem_t *m)
{
  int16_t off = (int16_t)m->off;
  uint16_t len = size_in_bytes(m->sz);

  // Global?
  if (m->sreg == REG_DS && !m->reg1 && !m->reg2) {
    s->kind = SYM_KIND_GLOBAL;
    s->off = off;
    s->len = len;
    s->name = nullptr;
    return true;
  }

  // Local var?
  if (m->sreg == REG_SS && m->reg1 == REG_BP && !m->reg2) {
    if (off < 0) {
      s->kind = SYM_KIND_LOCAL;
      s->off = off;
      s->len = len;
      s->name = nullptr;
    } else {
      s->kind = SYM_KIND_PARAM;
      s->off = off;
      s->len = len;
      s->name = nullptr;
    }
    return true;
  }

  return false;
}

bool sym_deduce_reg(sym_t *sym, int reg_id)
{
  uint16_t off, len;
  const char *name;

  switch (reg_id) {
    case REG_AX:    off =  0; len = 2; name = "AX";    break;
    case REG_CX:    off =  2; len = 2; name = "CX";    break;
    case REG_DX:    off =  4; len = 2; name = "DX";    break;
    case REG_BX:    off =  6; len = 2; name = "BX";    break;
    case REG_SP:    off =  8; len = 2; name = "SP";    break;
    case REG_BP:    off = 10; len = 2; name = "BP";    break;
    case REG_SI:    off = 12; len = 2; name = "SI";    break;
    case REG_DI:    off = 14; len = 2; name = "DI";    break;
    case REG_AL:    off =  0; len = 1; name = "AL";    break;
    case REG_CL:    off =  2; len = 1; name = "CL";    break;
    case REG_DL:    off =  4; len = 1; name = "DL";    break;
    case REG_BL:    off =  6; len = 1; name = "BL";    break;
    case REG_AH:    off =  1; len = 1; name = "AH";    break;
    case REG_CH:    off =  3; len = 1; name = "CH";    break;
    case REG_DH:    off =  5; len = 1; name = "DH";    break;
    case REG_BH:    off =  7; len = 1; name = "CH";    break;
    case REG_ES:    off = 16; len = 2; name = "ES";    break;
    case REG_CS:    off = 18; len = 2; name = "CS";    break;
    case REG_SS:    off = 20; len = 2; name = "SS";    break;
    case REG_DS:    off = 22; len = 2; name = "DS";    break;
    case REG_IP:    off = 24; len = 2; name = "IP";    break;
    case REG_FLAGS: off = 26; len = 2; name = "FLAGS"; break;
    default: return false;
  }

  sym->kind = SYM_KIND_REGISTER;
  sym->off  = (int16_t)off;
  sym->len  = len;
  sym->name = name;
  return true;
}

size_t sym_size_bytes(sym_t *s)
{
  return s->len;
}

std::string sym_name(sym_t *sym)
{
  if (sym->name) {
    return sym->name;
  }
  std::string s;

  switch (sym->kind) {
    case SYM_KIND_PARAM:
      s += std::format<"_param_%04x">((uint16_t)sym->off);
      break;
    case SYM_KIND_LOCAL:
      s += std::format<"_local_%04x">((uint16_t)-sym->off);
      break;
    case SYM_KIND_GLOBAL:
      s += std::format<"G_data_%04x">((uint16_t)sym->off);
      break;
    default:
      FAIL("Unknown sym kind: %d", sym->kind);
  }
  return s;
}

static bool sym_overlaps(sym_t *a, sym_t *b)
{
  // WLOG: Let a->off <= b->off
  if (b->off < a->off) {
    sym_t *tmp = a;
    a = b;
    b = tmp;
  }

  int16_t end = (int16_t)a->off + a->len;

  return
    a->kind == b->kind &&
    b->off < end;
}

symbols_t * symbols_new(void)
{
  symbols_t *s = (symbols_t*)calloc(1, sizeof(symbols_t));
  s->registers = symtab_new();
  s->globals   = symtab_new();
  s->params    = symtab_new();
  s->locals    = symtab_new();
  return s;
}

void symbols_delete(symbols_t *s)
{
  symtab_delete(s->registers);
  symtab_delete(s->globals);
  symtab_delete(s->params);
  symtab_delete(s->locals);
}

symtab_t * symtab_new(void)
{
  symtab_t *s = (symtab_t*)calloc(1, sizeof(symtab_t));
  s->n_var = 0;

  return s;
}

void symtab_delete(symtab_t *s)
{
  free(s);
}

static void symtab_add_merge(symtab_t *s, sym_t *sym)
{
  for (size_t i = 0; i < s->n_var; i++) {
    sym_t *cand = &s->var[i];
    if (!sym_overlaps(sym, cand)) continue;

    // Overlaps: grow to encapsulate both!

    int16_t new_start = MIN(sym->off, cand->off);
    int16_t new_end   = MAX(sym->off + sym->len, cand->off + cand->len);
    int new_len   = new_end - new_start;

    // Update sym
    sym->off = new_start;
    sym->len = new_len;

    // Remove the candidate (avoid duplicates)
    s->var[i] = s->var[--s->n_var];
    i--;
  }

  assert(s->n_var < ARRAY_SIZE(s->var));
  s->var[s->n_var++] = *sym;
}

static symref_t symtab_find(symtab_t *s, sym_t *deduced_sym)
{
  symref_t ref = {};

  for (size_t i = 0; i < s->n_var; i++) {
    sym_t *cand = &s->var[i];
    if (sym_overlaps(deduced_sym, cand)) {
      assert(cand->off <= deduced_sym->off);
      ref.symbol = cand;
      ref.off    = deduced_sym->off - cand->off;
      ref.len    = deduced_sym->len;
      break;
    }
  }

  return ref;
}

typedef struct iter_impl iter_impl_t;
struct __attribute__((aligned(16))) iter_impl
{
  symtab_t * s;
  size_t     idx;
  char       _extra[16];
};
static_assert(sizeof(iter_impl_t) == sizeof(symtab_iter_t), "");
static_assert(alignof(iter_impl_t) == alignof(symtab_iter_t), "");

void symtab_iter_begin(symtab_iter_t *_it, symtab_t *s)
{
  iter_impl_t *it = (iter_impl_t*)_it;
  it->s = s;
  it->idx = 0;
}

sym_t * symtab_iter_next(symtab_iter_t *_it)
{
  iter_impl_t *it = (iter_impl_t*)_it;
  if (it->idx >= it->s->n_var) return nullptr;
  return &it->s->var[it->idx++];
}

bool symbols_insert_deduced(symbols_t *s, sym_t *deduced_sym)
{
  switch (deduced_sym->kind) {
    case SYM_KIND_REGISTER: symtab_add_merge(s->registers, deduced_sym); break;
    case SYM_KIND_PARAM:    symtab_add_merge(s->params,    deduced_sym); break;
    case SYM_KIND_LOCAL:    symtab_add_merge(s->locals,    deduced_sym); break;
    case SYM_KIND_GLOBAL: {
      // Globals are special in that we don't merge them in. We require that globals
      // are set up via a config file. So, here, we simply verify that our deduced
      // symbol cooresponds to some pre-configured global
      symref_t ref = symtab_find(s->globals, deduced_sym);
      if (!ref.symbol) {
        //const char *name = sym_name(deduced_sym);
        //FAIL("Failed to find global for '%s'", name);
        return false;
      }
    } break;
    default: FAIL("Unknown symbol kind: %d", deduced_sym->kind);
  }

  return true;
}

symref_t symbols_find_ref(symbols_t *s, sym_t *deduced_sym)
{
  switch (deduced_sym->kind) {
    case SYM_KIND_REGISTER: return symtab_find(s->registers, deduced_sym);
    case SYM_KIND_PARAM:    return symtab_find(s->params,    deduced_sym);
    case SYM_KIND_LOCAL:    return symtab_find(s->locals,    deduced_sym);
    case SYM_KIND_GLOBAL:   return symtab_find(s->globals,   deduced_sym);
    default: FAIL("Unknown symbol kind: %d", deduced_sym->kind);
  }
}

symref_t symbols_find_mem(symbols_t *s, operand_mem_t *mem)
{
  sym_t deduced_sym[1];
  if (!sym_deduce(deduced_sym, mem)) {
    symref_t ref = {};
    return ref;
  }
  return symbols_find_ref(s, deduced_sym);
}

symref_t symbols_find_reg(symbols_t *s, int reg_id)
{
  sym_t deduced_sym[1];
  if (!sym_deduce_reg(deduced_sym, reg_id)) {
    symref_t ref = {};
    return ref;
  }
  return symbols_find_ref(s, deduced_sym);
}

void symbols_add_global(symbols_t *s, const char *name, uint16_t offset, uint16_t len)
{
  sym_t sym[1] = {{}};
  sym->kind = SYM_KIND_GLOBAL;
  sym->off  = (int16_t)offset;
  sym->len  = len;
  sym->name = name;

  symtab_t *symtab = s->globals;
  assert(symtab->n_var < ARRAY_SIZE(symtab->var));
  symtab->var[symtab->n_var++] = *sym;
}

bool symref_matches(symref_t *a, symref_t *b)
{
  return
    a->symbol == b->symbol &&
    a->off == b->off &&
    a->len == b->len;
}
