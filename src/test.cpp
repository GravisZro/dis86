#ifdef ENABLE_TESTS

namespace region { extern void test(void); }
namespace bin::instr_fmt { extern void test(void); }
namespace bin::decode { extern void test(void); }
namespace bsl { extern void test(void); }
namespace dvec { extern void test(void); }

int main(void)
{
  region::test();
  bin::instr_fmt::test();
  bin::decode::test();
  bsl::test();
  dvec::test();
  return 0;
}

#endif
