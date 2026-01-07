#include "bsl.h"
#include <assert.h>
#include <stdalign.h>
#include <cstdint>
#include <string.h>

#define HAX_FAIL(...) do { fprintf(stderr, "FAIL (%s:%d): ", __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); abort(); } while(0)

namespace bsl
{
  enum class token_e : uint8_t
  {
    invalid = 254,
    eof = 255,
    string = 0,
    open = '{',
    close = '}'
  };

  struct parser_t
  {
    const uint8_t*  buf;
    size_t          sz;
    size_t          idx;

    token_e         tok_type;
    const uint8_t*  tok_buf;
    size_t          tok_len;
  };

  static void parser_init(parser_t *p, const dynarray& buf)
  {
    p->buf = buf.data();
    p->sz = buf.size();
    p->idx = 0;
  }

  static inline bool is_white(uint8_t c)   { return c == ' ' || c == '\t' || c == '\n'; }
  static inline bool is_visible(uint8_t c) { return 33 <= c && c <= 126; }

  static void parser_skip_white(parser_t *p)
  {
    for (; p->idx < p->sz; p->idx++) {
      uint8_t c = p->buf[p->idx];
      if (!is_white(c)) break;
    }
  }

  static uint8_t parser_advance(parser_t *p)
  {
    if (p->idx < p->sz)
      return p->idx++;
    return 0;
  }

  static uint8_t parser_char(parser_t *p)
  {
    return (p->idx == p->sz) ? '\0' : p->buf[p->idx];
  }

  static void parser_tok_next(parser_t *p)
  {
    // skip all whitespace
    parser_skip_white(p);

    // token end?
    if (p->idx == p->sz) {
      p->tok_type = token_e::eof;
      p->tok_len = 0;
      return;
    }

    p->tok_buf = &p->buf[p->idx];
    uint8_t c = *p->tok_buf;

    // token punctuation
    if (c == '{' || c == '}') {
      parser_advance(p);
      p->tok_type = token_e(c);
      p->tok_len = 1;
      return;
    }

    // token str (quoted)
    if (c == '"') {
      p->tok_type = token_e::string;
      while (1) {
        parser_advance(p);
        c = parser_char(p);
        if (c == '\0')
          HAX_FAIL("REACHED EOF WHILE INSIDE A QUOTED STRING");
        if (c == '"') break; // Found!!
      }

      // Advance past the quote
      parser_advance(p);

      // Remove the quotes from the output
      p->tok_buf++;  // skip starting '"'
      p->tok_len = &p->buf[p->idx] - p->tok_buf - 1; // skip ending '"'
      return;
    }

    // token str
    if (is_visible(c)) {
      p->tok_type = token_e::string;
      while (is_visible(c)) {
        parser_advance(p);
        c = parser_char(p);
      }
      p->tok_len = &p->buf[p->idx] - p->tok_buf;
      return;
    }

    HAX_FAIL("BAD TOK");
  }

  static char* parser_tok_str(parser_t *p)
  {
    char *s = (char*)malloc(p->tok_len+1);
    memcpy(s, p->tok_buf, p->tok_len);
    s[p->tok_len] = '\0';
    return s;
  }



  static node_t * node_new(void)
  {
    node_t *n = (node_t*)malloc(sizeof(node_t));
    if (!n) return nullptr;

    n->kv_len = 0;
    n->kv_cap = 8;
    n->kv_arr = (keyval_t*)malloc(n->kv_cap * sizeof(keyval_t));
    n->toplevel = 0;

    if (!n->kv_arr) {
      free_node(n);
      return nullptr;
    }

    return n;
  }

  static void node_delete(node_t *n)
  {
    for (size_t i = 0; i < n->kv_len; i++) {
      keyval_t *kv = &n->kv_arr[i];
      if (kv->type == node_e::string) {
        ::free(kv->val.string);
      } else if (kv->type == node_e::node) {
        node_delete(kv->val.node);
      }
    }
    ::free(n->kv_arr);
    free_node(n);
  }

  static void node_append(node_t *n, keyval_t kv /* value moved into this node */)
  {
    if (n->kv_len == n->kv_cap) { // realloc?
      n->kv_cap *= 2;
      n->kv_arr = (keyval_t*)realloc(n->kv_arr, n->kv_cap * sizeof(keyval_t));
      if (!n->kv_arr) HAX_FAIL("CANNOT REALLOC");
    }

    assert(n->kv_len < n->kv_cap);
    n->kv_arr[n->kv_len++] = kv;
  }

  // forward decl needed for mutually recursively dependent parser
  static node_t *parse_node(parser_t *p);

  // value = str | "{" node "}"
  static node_val_t parse_value(parser_t *p, node_e& out_type)
  {
    node_val_t rval;
    if (p->tok_type == token_e::string) {
      rval.string = parser_tok_str(p);
      parser_tok_next(p);
      out_type = node_e::string;
      return rval;
    }

    else if (p->tok_type == token_e::open) {
      parser_tok_next(p);
      rval.node = parse_node(p);
      if (p->tok_type != token_e::close)
        HAX_FAIL("Expected closing '}'");
      parser_tok_next(p);
      out_type = node_e::node;
      return rval;
    }

    else {
      HAX_FAIL("Expected value to start with either a string or '{', got [0x%x]", int(p->tok_type));
    }

  }

  // keyval = str value
  static bool parse_keyval(parser_t *p, keyval_t& out_kv)
  {
    if (p->tok_type != token_e::string) return false;
    char* key = parser_tok_str(p);
    parser_tok_next(p);

    node_e type;
    node_val_t val = parse_value(p, type);

    out_kv.type = type;
    out_kv.key  = key;
    out_kv.val  = val;

    return true;
  }

  // node = keyval*
  static node_t *parse_node(parser_t *p)
  {
    node_t * node = node_new();

    while (1) {
      keyval_t kv;
      if (!parse_keyval(p, kv)) break;
      node_append(node, kv);
    }

    return node;
  }

  node_t* parse_new(const dynarray& buf, error_e *opt_err)
  {
    parser_t p[1];
    parser_init(p, buf);
    parser_tok_next(p);

    node_t * node = parse_node(p);
    if (p->tok_type != token_e::eof)
      HAX_FAIL("EXPECTED EOF");

    if (opt_err)
      *opt_err = error_e::success;
    node->toplevel = 1;
    return node;
  }

  void free_node(node_t *bsl)
  {
    node_t *node = (node_t*)bsl;
    if (!node->toplevel) {
      fprintf(stderr, "ERR: FATAL CODING BUG DETECTED. INVALID TO DELETE INTERNAL NODES.");
      abort();
    }

    node_delete(node);
  }

  static node_val_t node_get(node_t *node, const char *key, size_t key_len, node_e& type)
  {
    for (size_t i = 0; i < node->kv_len; i++) {
      keyval_t *kv = &node->kv_arr[i];

      if (strlen(reinterpret_cast<const char*>(kv->key)) != key_len) continue;
      if (0 != memcmp(kv->key, key, key_len)) continue;

      // Found!
      type = kv->type;
      return kv->val;
    }
    return { nullptr };
  }

  node_val_t get_generic(node_t *bsl, const char *key, node_e& type)
  {
    node_t *node = bsl;

    if (!key || !*key)
      return { nullptr };

    const char * ptr = key;
    while (1) {
      const char * end = ptr;
      while (*end && *end != '.') end++;
      size_t len = end - ptr;

      node_e sub_type = node_e::invalid;
      node_val_t val = node_get(node, ptr, len, sub_type);
      if (val.string == nullptr)
        return { nullptr }; // Not Found

      if (*end == '\0') {

        type = sub_type;
        return val;
      }

      if (sub_type != node_e::node)
        return { nullptr }; // Not a node type

      node = val.node;
      ptr = end+1;
    }
  }

  const char * get_str(node_t *bsl, const char *key)
  {
    node_e type = node_e::invalid;
    node_val_t val = get_generic(bsl, key, type);
    if (val.string == nullptr || type != node_e::string) return nullptr;
    return val.string;
  }

  node_t * get_node(node_t *bsl, const char *key)
  {
    node_e type = node_e::invalid;
    node_val_t val = get_generic(bsl, key, type);
    if (val.node == nullptr || type != node_e::node)
      return nullptr;
    return val.node;
  }

  struct alignas(16) iter_impl_t
  {
    node_t * node;
    size_t       idx;
    char         _extra[16];
  };
  static_assert(sizeof(iter_impl_t) == sizeof(iter_t), "");
  static_assert(alignof(iter_impl_t) == alignof(iter_t), "");

  void iter_begin(iter_t *_it, node_t *bsl)
  {
    iter_impl_t *it = (iter_impl_t*)_it;
    it->node = (node_t*)bsl;
    it->idx  = 0;
  }

  bool iter_next(iter_t *_it, node_e& _type, const char **_key, node_val_t& _val)
  {
    iter_impl_t * it   = (iter_impl_t*)_it;
    node_t *  node = it->node;

    if (it->idx == node->kv_len)
      return false;

    keyval_t *kv = &node->kv_arr[it->idx++];

    _type = kv->type;
    *_key  = kv->key;
    _val  = kv->val;

    return true;
  }
}
