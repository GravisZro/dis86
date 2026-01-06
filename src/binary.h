#include "header.h"

struct binary_t
{
  uint8_t *   mem;
  size_t len;
  size_t idx;
  size_t base_addr;
};

static inline void binary_init(binary_t *b, size_t base_addr, char *mem, size_t len)
{
  b->mem = (uint8_t*)malloc(len);
  memcpy(b->mem, mem, len);
  b->len = len;
  b->idx = base_addr;
  b->base_addr = base_addr;
}

static inline uint8_t binary_byte_at(binary_t *b, size_t idx)
{
  if (idx < b->base_addr) FAIL("Binary access below start of region");
  if (idx >= b->base_addr + b->len) FAIL("Binary access beyond end of region");
  return b->mem[idx - b->base_addr];
}

static inline uint8_t binary_peek_uint8_t(binary_t *b)
{
  uint8_t byte = binary_byte_at(b, b->idx);
  return byte;
}

static inline void binary_advance_uint8_t(binary_t *b)
{
  b->idx++;
}

static inline uint8_t binary_fetch_uint8_t(binary_t *b)
{
  uint8_t byte = binary_peek_uint8_t(b);
  binary_advance_uint8_t(b);
  return byte;
}

static inline uint16_t binary_fetch_uint16_t(binary_t *b)
{
  uint8_t low = binary_fetch_uint8_t(b);
  uint8_t high = binary_fetch_uint8_t(b);
  return (uint16_t)high << 8 | (uint16_t)low;
}

static inline size_t binary_baseaddr(binary_t *b)
{
  return b->base_addr;
}

static inline size_t binary_location(binary_t *b)
{
  return b->idx;
}

static inline size_t binary_length(binary_t *b)
{
  return b->len;
}

static inline void binary_dump(binary_t *b)
{
  printf("BINARY DUMP LOCATION %zx: ", b->idx);
  size_t end = MIN(b->idx + 16, b->base_addr + b->len);
  for (size_t idx = b->idx; idx < end; idx++) {
    printf("%02x ", binary_byte_at(b, idx));
  }
  printf("\n");
}
