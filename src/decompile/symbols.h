#pragma once

#include <cstdint>
#include <unistd.h>
#include <string>

#define NAME_MAX 128


struct __attribute__((aligned(16))) symtab_iter_t { char _opaque[32]; };

enum {
  SYM_KIND_REGISTER,
  SYM_KIND_PARAM,
  SYM_KIND_LOCAL,
  SYM_KIND_GLOBAL,
};

struct sym_t
{
  int          kind;
  int16_t          off;
  uint16_t          len;              // in bytes
  const char * name;             // optional (default name is constructed otherwise)
};


#define SYMTAB_MAX_SIZE 4096

struct symtab_t
{
  // TODO: REPLACE WITH A HASHTABLE
  size_t n_var;
  sym_t var[SYMTAB_MAX_SIZE];
};

struct symref_t
{
  sym_t * symbol;  // nullptr if the ref doesn't point anywhere
  uint16_t     off;     // offset into this symbol
  uint16_t     len;     // length from the offset
};


bool         sym_deduce(sym_t *v, operand_mem_t *mem);
bool         sym_deduce_reg(sym_t *sym, int reg_id);
std::string sym_name(sym_t *v);
size_t       sym_size_bytes(sym_t *v);

struct symbols_t
{
  symtab_t * registers;
  symtab_t * globals;
  symtab_t * params;
  symtab_t * locals;
};

symbols_t * symbols_new(void);
void        symbols_delete(symbols_t *s);
bool        symbols_insert_deduced(symbols_t *s, sym_t *deduced_sym);
symref_t    symbols_find_ref(symbols_t *s, sym_t *deduced_sym);
symref_t    symbols_find_mem(symbols_t *s, operand_mem_t *mem);
symref_t    symbols_find_reg(symbols_t *s, int reg_id);
void        symbols_add_global(symbols_t *s, const char *name, uint16_t offset, uint16_t len);

bool symref_matches(symref_t *a, symref_t *b);

symtab_t * symtab_new(void);
void       symtab_delete(symtab_t *s);

void    symtab_iter_begin(symtab_iter_t *it, symtab_t *s);
sym_t * symtab_iter_next(symtab_iter_t *it);
