#include "bsl.h"
#include <assert.h>
#include <stdalign.h>
#include <cstdint>
#include <string.h>
#include <cctype>
#include <iostream>

#define HAX_FAIL(...) do { fprintf(stderr, "FAIL (%s:%d): ", __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); abort(); } while(0)

namespace bsl
{

  static inline bool is_visible(char c)
  { return std::isprint(c) && !std::isspace(c); }


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
    parser_t(const std::string& dynbuf)
    {
      buf = dynbuf.data();
      sz = dynbuf.size();
      idx = 0;
    }

    void skip_white(void)
    {
      while(idx < sz && std::isspace(buf[idx]))
        idx++;
    }

    uint8_t advance(void)
    {
      if (idx < sz)
        return idx++;
      return 0;
    }

    char current(void) { return (idx >= sz) ? '\0' : buf[idx]; }

    void tok_next(void);

    std::optional<std::string> tok_str(void)
    {
      if(tok_len)
        return std::string(tok_buf, tok_len);
      return {};
    }


    node_val_t parse_value(node_e& out_type);
    std::shared_ptr<node_t> parse_node(void);
    bool parse_keyval(keyval_t& out_kv);

    const char* buf;
    std::size_t sz;
    std::size_t idx;

    token_e     tok_type;
    const char* tok_buf;
    std::size_t tok_len;
  };

  void parser_t::tok_next(void)
  {
    // skip all whitespace
    skip_white();

    // token end?
    if (idx == sz) {
      tok_type = token_e::eof;
      tok_len = 0;
      return;
    }

    tok_buf = &buf[idx];
    char c = *tok_buf;

    // token punctuation
    if (c == '{' || c == '}') {
      advance();
      tok_type = token_e(c);
      tok_len = 1;
      return;
    }

    // token str (quoted)
    if (c == '"') {
      tok_type = token_e::string;
      while (1) {
        advance();
        c = current();
        if (c == '\0')
          HAX_FAIL("REACHED EOF WHILE INSIDE A QUOTED STRING");
        if (c == '"') break; // Found!!
      }

      // Advance past the quote
      advance();

      // Remove the quotes from the output
      tok_buf++;  // skip starting '"'
      tok_len = &buf[idx] - tok_buf - 1; // skip ending '"'
      return;
    }

    // token str
    if (is_visible(c)) {
      tok_type = token_e::string;
      while (is_visible(c)) {
        advance();
        c = current();
      }
      tok_len = &buf[idx] - tok_buf;
      return;
    }

    HAX_FAIL("BAD TOK");
  }

  // value = str | "{" node "}"
  node_val_t parser_t::parse_value(node_e& out_type)
  {
    node_val_t rval;
    if (tok_type == token_e::string)
    {
      rval.string = tok_str();
      tok_next();
      out_type = node_e::string;
    }
    else if (tok_type == token_e::open)
    {
      tok_next();
      rval.node = parse_node();
      if (tok_type != token_e::close)
        HAX_FAIL("Expected closing '}'");
      tok_next();
      out_type = node_e::node;
    }
    else
    {
      HAX_FAIL("Expected value to start with either a string or '{', got [0x%x]", int(tok_type));
    }
    return rval;
  }

  // keyval = str value
  bool parser_t::parse_keyval(keyval_t& out_kv)
  {
    if (tok_type != token_e::string)
      return false;

    auto key = tok_str();
    if(!key)
      return false;
    tok_next();

    node_e type;
    node_val_t val = parse_value(type);

    out_kv.type = type;
    out_kv.key  = *key;
    out_kv.val  = val;
    return true;
  }

  // node = keyval*
  std::shared_ptr<node_t> parser_t::parse_node(void)
  {
    std::shared_ptr<node_t> node(new node_t());
    keyval_t kv;
    while (parse_keyval(kv))
      node->kv_arr.push_back(kv);
    return node;
  }

  std::shared_ptr<node_t> parse_new(const std::string& buf, error_e *opt_err)
  {
    parser_t p(buf);
    p.tok_next();

    std::shared_ptr<node_t> node = p.parse_node();
    if (p.tok_type != token_e::eof)
      HAX_FAIL("EXPECTED EOF");

    if (opt_err)
      *opt_err = error_e::success;
    return node;
  }

  static std::optional<node_val_t> node_get(std::shared_ptr<node_t> node, const std::string& key, node_e& type)
  {
    assert(!node->kv_arr.empty());
    for(keyval_t& kv : node->kv_arr)
      if(kv.key == key)
      {
        type = kv.type;
        return kv.val;
      }
    return {};
  }

  std::shared_ptr<node_t> keyval_t::as_node(void) const
  {
    if(type != node_e::node)
      return nullptr;
    return val.node;
  }


  std::optional<node_val_t> node_t::get_generic(const std::string& key, node_e& type)
  {
    if(key.empty())
      return {};

    std::shared_ptr<node_t> node = shared_from_this();
    const char* ptr = key.data();
    const char* const eos = key.data() + key.size();
    for(;;)
    {
      const char* end = ptr;
      while (end < eos && *end != '.')
        end++;
      std::size_t len = end - ptr;
      node_e sub_type = node_e::invalid;
      auto val = node_get(node, std::string(ptr, len), sub_type);
      if (!val)
        return {}; // Not Found

      if (end == eos)
      {
        type = sub_type;
        return val;
      }

      if (sub_type != node_e::node)
        return {}; // Not a node type

      node = val->node;
      ptr = end + 1;
    }
  }

  std::optional<std::string> node_t::get_str(const std::string& key)
  {
    node_e type = node_e::invalid;
    auto val = get_generic(key, type);
    if (!val || type != node_e::string)
      return {};
    return val->string;
  }

  std::shared_ptr<node_t> node_t::get_node(const std::string& key)
  {
    node_e type = node_e::invalid;
    auto val = get_generic(key, type);
    if (!val || type != node_e::node)
      return nullptr;
    return val->node;
  }

/*
  struct alignas(16) iter_impl_t
  {
    node_t* node;
    std::size_t  idx;
    char    _extra[16];
  };
  static_assert(sizeof(iter_impl_t) == sizeof(iter_t), "");
  static_assert(alignof(iter_impl_t) == alignof(iter_t), "");

  void iter_begin(iter_t *_it, node_t *bsl)
  {
    iter_impl_t *it = (iter_impl_t*)_it;
    it->node = (node_t*)bsl;
    it->idx  = 0;
  }

  bool iter_next(iter_t *_it, node_e& _type, std::string* _key, node_val_t& _val)
  {
    iter_impl_t * it   = (iter_impl_t*)_it;
    node_t *  node = it->node;

    if (it->idx >= node->kv_arr.size())
      return false;

    keyval_t *kv = &node->kv_arr[it->idx++];

    _type = kv->type;
    *_key  = kv->key;
    _val  = kv->val;

    return true;
  }
*/

}
