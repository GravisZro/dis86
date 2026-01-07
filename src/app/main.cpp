#include <stdio.h>
#include <string.h>

namespace disassembler
{
  extern int main(int argc, char *argv[]);
}
namespace decompiler
{
  extern int main(int argc, char *argv[]);
}

static void print_help(FILE *f, const char *appname)
{
  fprintf(f, "usage: %s <mode> [<MODE-SPECIFIC-OPTIONS>]\n", appname);
  fprintf(stderr, "\n");
  fprintf(stderr, "MODES:\n");
  fprintf(stderr, "  dis       disassemble the binary and emit intel syntax\n");
  fprintf(stderr, "  decomp    decompile the binary\n");
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    print_help(stderr, argv[0]);
    return 1;
  }
  const char *mode = argv[1];

  if (0) {}
  else if (0 == strcmp(mode, "dis"))    return disassembler::main(argc, argv);
  else if (0 == strcmp(mode, "decomp")) return decompiler::main(argc, argv);

  fprintf(stderr, "Error: Unknown mode '%s'", mode);
  print_help(stderr, argv[0]);
  return 2;
}
