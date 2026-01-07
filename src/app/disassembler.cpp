#include "dis86.h"
#include "dis86.h"
#include "segoff.h"
#include "cmdarg/cmdarg.h"

#include "common/common.h"
#include <cstdint>
#include <string>

namespace disassembler
{
  static dis86_t *dis_exit = nullptr;
  static void on_fail()
  {
    if (!dis_exit) return;
    binary_dump(dis_exit->b);
  }

  static void print_help(FILE *f, const char *appname)
  {
    fprintf(f, "usage: %s dis OPTIONS\n", appname);
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  --binary       path to binary on the filesystem (required)\n");
    fprintf(stderr, "  --start-addr   start seg:off address (required)\n");
    fprintf(stderr, "  --end-addr     end seg:off address (required)\n");
  }

  static bool cmdarg_segoff(int * argc, char *** argv, const char * name, segoff_t *_out)
  {
    const char *s;
    if (!cmdarg_string(argc, argv, name, &s)) return false;

    *_out = parse_segoff(s);
    return true;
  }

  int main(int argc, char *argv[])
  {
    atexit(on_fail);

    const char * binary = nullptr;
    segoff_t     start  = {};
    segoff_t     end    = {};

    bool found;

    found = cmdarg_string(&argc, &argv, "--binary", &binary);
    if (!found) { print_help(stderr, argv[0]); return 3; }

    found = cmdarg_segoff(&argc, &argv, "--start-addr", &start);
    if (!found) { print_help(stderr, argv[0]); return 3; }

    found = cmdarg_segoff(&argc, &argv, "--end-addr", &end);
    if (!found) { print_help(stderr, argv[0]); return 3; }

    size_t start_idx = segoff_abs(start);
    size_t end_idx = segoff_abs(end);

    dynarray mem = read_file(binary);

    dis86_t *d = dis86_new(start_idx, mem.segment(start_idx, end_idx - start_idx));
    if (!d) FAIL("Failed to allocate dis86 instance");

    dis_exit = d;

    std::string s;
    dis86_instr_t* ins = nullptr;
    while (ins = dis86_next(d), ins != nullptr)
    {
      s = dis86_print_intel_syntax(d, ins, true);
      printf("%s\n", s.c_str());
      s.clear();
    }

    dis_exit = nullptr;
    dis86_delete(d);
    return 0;
  }
}
