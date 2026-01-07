#include "common.h"

#include <fstream>
#include <cassert>

dynarray read_file(const std::string& filename)
{
  dynarray buffer;
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if(std::streamsize size = file.tellg(); size > 0)
  {
    file.seekg(0, std::ios::beg);
    buffer.init(size);
    assert(file.read(reinterpret_cast<char*>(buffer.data()), size));
  }
  return buffer;
}
/*
static inline char *read_file(const char *filename, size_t *out_sz)
{
  FILE *fp = fopen(filename, "r");
  if (!fp) FAIL("Failed to open: '%s'", filename);

  fseek(fp, 0, SEEK_END);
  size_t file_sz = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *mem = (char*)malloc(file_sz);
  if (!mem) FAIL("Failed to allocate");

  size_t n = fread(mem, 1, file_sz, fp);
  if (n != file_sz) FAIL("Failed to read");
  fclose(fp);

  if (out_sz) *out_sz = file_sz;
  return mem;
}
*/
