#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>

#include "bsl.h"

namespace bsl
{  
  static inline void test_fail(const char* format, ...)
  {
    std::va_list args;
    va_start(args, format);
    fprintf(stderr, "TEST FAIL: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    abort();
  }

  static inline std::shared_ptr<node_t> parse(const std::string& data)
  {
    error_e err = error_e::success;
    std::shared_ptr<node_t> b = parse_new(data, &err);
    if (!b)
      test_fail("'%s'", data.data());
    return b;
  }

  static inline void get_helper(std::shared_ptr<node_t> b,
                                const std::string& key,
                                const std::string& exp_val,
                                bool succeed = true)
  {
    auto val = b->get_str(key);
    if (succeed)
    {
      if (!val)
        test_fail("Failed to get string key: '%s'", key.data());
      else if (*val != exp_val)
        test_fail("Mismatch value: expected '%s', got '%s'", exp_val.data(), val->data());
    }
    else if (!succeed && val)
      test_fail("Expected failure, but got success on key: '%s'", key.data());
  }

  static inline void get_node_helper(std::shared_ptr<node_t> b, const std::string& key, bool succeed = true)
  {
    std::shared_ptr<node_t> val = b->get_node(key);
    if (succeed && !val)
      test_fail("Failed to get node key: '%s'", key.data());
    else if (!succeed && val)
      test_fail("Expected failure, but got success on key: '%s'", key.data());
  }


  static inline void get_pass(std::shared_ptr<node_t> b, const std::string& key, const std::string& val)
    { return get_helper(b, key, val, true); }

  static inline void get_fail(std::shared_ptr<node_t> b, const std::string& key)
    { return get_helper(b, key, "", false); }

  static inline void get_node_pass(std::shared_ptr<node_t> b, const std::string& key)
    { return get_node_helper(b, key, true); }

  static void test_1(void)
  {
    std::string data = "foo bar";
    std::shared_ptr<node_t> b = parse(data);
    get_pass(b, "foo", "bar");
    get_fail(b, "foo1");
    std::cout << "BSL test 1: passed" << std::endl;
  }

  static void test_2(void)
  {
    std::string data = "foo bar good stuff   ";
    std::shared_ptr<node_t> b = parse(data);
    get_pass(b, "foo", "bar");
    get_pass(b, "good", "stuff");
    get_fail(b, "foo1");
    std::cout << "BSL test 2: passed" << std::endl;
  }

  static void test_3(void)
  {
    std::string data = "top {foo bar baz {} } top2 r ";
    std::shared_ptr<node_t> b = parse(data);
    get_pass(b, "top.foo", "bar");
    get_fail(b, "top.foo.baz");
    get_node_pass(b, "top.baz");
    get_pass(b, "top2", "r");
    std::cout << "BSL test 3: passed" << std::endl;
  }

  static void test_4(void)
  {
    std::string data = "top \"foo bar\" bot g quote \"{ key val }\"";
    std::shared_ptr<node_t> b = parse(data);
    get_pass(b, "top", "foo bar");
    get_pass(b, "bot", "g");
    get_pass(b, "quote", "{ key val }");
    std::cout << "BSL test 4: passed" << std::endl;
  }

  void test(void)
  {
    test_1();
    test_2();
    test_3();
    test_4();
  }
}
