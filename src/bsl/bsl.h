#pragma once

/* BSL: Barebones Specification Language */

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <string>
#include <memory>
#include <optional>

//#include "common/dynarray.h"

namespace bsl
{

  //struct __attribute__((aligned(16))) iter_t { char _opaque[32]; };


  enum class node_e : uint8_t
  {
    invalid = 255,
    string  = 0,  // char *
    node = 1,  // node_t *
  };

  struct node_val_t
  {
    std::shared_ptr<struct node_t> node;
    std::optional<std::string> string;
  };

  struct keyval_t
  {
    std::shared_ptr<struct node_t> as_node(void) const;
    node_e      type; // BSL_TYPE_*
    std::string key;
    node_val_t  val;
  };

  enum error_e
  {
    success,
    parse_error,
  };

  struct node_t : public std::enable_shared_from_this<node_t>
  {
    std::optional<node_val_t>   get_generic (const std::string& key, node_e& type);
    std::optional<std::string>  get_str     (const std::string& key);
    std::shared_ptr<node_t>     get_node    (const std::string& key);

    std::vector<keyval_t> kv_arr;
  };

  std::shared_ptr<node_t> parse_new(const std::string& buf, error_e *opt_err = nullptr);

//  void        iter_begin(iter_t *it, node_t *bsl);
//  bool        iter_next(iter_t *it, node_e& _type, const char **_key, node_val_t& _val);
}
