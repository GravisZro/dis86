#pragma once

/* BSL: Barebones Specification Language */

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>

#include "common/dynarray.h"

namespace bsl
{

  struct __attribute__((aligned(16))) iter_t { char _opaque[32]; };


  enum class node_e : uint8_t
  {
    invalid = 255,
    string  = 0,  // char *
    node = 1,  // node_t *
  };

  union node_val_t
  {
    struct node_t* node;
    char* string;
  };

  struct keyval_t
  {
    node_e  type; // BSL_TYPE_*
    char*   key;
    node_val_t val;
  };

  struct node_t
  {
    keyval_t * kv_arr;
    size_t         kv_len;
    size_t         kv_cap;
    int            toplevel;
  };

  enum error_e
  {
    success,
    parse_error,
  };


  node_t*  parse_new(const dynarray& buf, error_e *opt_err = nullptr);
  void     free_node(node_t *bsl);

  node_val_t get_generic(node_t *bsl, const char *key, node_e& type);
  const char * get_str(node_t *bsl, const char *key);
  node_t*  get_node(node_t *bsl, const char *key);

  void         iter_begin(iter_t *it, node_t *bsl);
  bool         iter_next(iter_t *it, node_e& _type, const char **_key, node_val_t& _val);

}
