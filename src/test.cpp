#ifdef ENABLE_TESTS

namespace region { extern void test(void); }
namespace instr_fmt { extern void test(void); }
namespace decode { extern void test(void); }
namespace bsl { extern void test(void); }

int main(void)
{
  region::test();
  instr_fmt::test();
  decode::test();
  bsl::test();
  return 0;
}

#endif
