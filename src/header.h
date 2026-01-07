#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

//static inline void bin_dump_and_abort();

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#define FAIL(...) do { fprintf(stderr, "FAIL: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(42); } while(0)
#define UNIMPL() FAIL("UNIMPLEMENTED: %s:%d", __FILE__, __LINE__)


static inline void hexdump(uint8_t *mem, size_t len)
{
  size_t idx = 0;
  while (idx < len) {
    size_t line_end = MIN(idx+16, len);
    for (; idx < line_end; idx++) {
      printf("%02x ", mem[idx]);
    }
    printf("\n");
  }
}

static uint64_t parse_hex_uint64_t(const char *s, size_t len)
{
  if (len > 16) FAIL("Hex string too long to fit in uint64_t");

  uint64_t ret = 0;
  for (size_t i = 0; i < len; i++) {
    char c = s[i];
    if ('0' <= c && c <= '9') {
      ret = ret*16 + (c-'0');
    } else if ('a' <= c && c <= 'f') {
      ret = ret*16 + (c-'a'+10);
    } else if ('A' <= c && c <= 'F') {
      ret = ret*16 + (c-'A'+10);
    } else {
      FAIL("Invalid hex string: '%.*s'", (int)len, s);
    }
  }

  return ret;
}

static uint32_t parse_hex_uint32_t(const char *s, size_t len)
{
  if (len > 8) FAIL("Hex string too long to fit in uint16_t");
  return (uint32_t)parse_hex_uint64_t(s, len);
}

static uint16_t parse_hex_uint16_t(const char *s, size_t len)
{
  if (len > 4) FAIL("Hex string too long to fit in uint16_t");
  return (uint16_t)parse_hex_uint64_t(s, len);
}

static uint8_t parse_hex_uint8_t(const char *s, size_t len)
{
  if (len > 2) FAIL("Hex string too long to fit in uint16_t");
  return (uint16_t)parse_hex_uint64_t(s, len);
}

static inline bool parse_bytes_uint64_t(const char *buf, size_t len, uint64_t *_num)
{
  if (len == 0) return false;

  uint64_t num = 0;
  for (size_t i = 0; i < len; i++) {
    char c = buf[i];
    if (!('0' <= c && c <= '9')) return false; // not a decimal digit

    uint64_t next_num = 10*num + (uint64_t)(c-'0');
    if (next_num < num) return false; // overflow!
    num = next_num;
  }

  *_num = num;
  return true;
}

static inline bool parse_bytes_uint32_t(const char *buf, size_t len, uint32_t *_num)
{
  uint64_t num;
  if (!parse_bytes_uint64_t(buf, len, &num)) return false;
  if ((uint64_t)(uint32_t)num != num) return false;
  *_num = (uint32_t)num;
  return true;
}

static inline bool parse_bytes_uint16_t(const char *buf, size_t len, uint16_t *_num)
{
  uint64_t num;
  if (!parse_bytes_uint64_t(buf, len, &num)) return false;
  if ((uint64_t)(uint16_t)num != num) return false;
  *_num = (uint16_t)num;
  return true;
}

static inline bool parse_bytes_uint8_t(const char *buf, size_t len, uint8_t *_num)
{
  uint64_t num;
  if (!parse_bytes_uint64_t(buf, len, &num)) return false;
  if ((uint64_t)(uint8_t)num != num) return false;
  *_num = (uint8_t)num;
  return true;
}

static inline bool parse_bytes_int64_t(const char *buf, size_t len, int64_t *_num)
{
  if (len == 0) return false;

  bool neg = false;
  if (buf[0] == '-') {
    neg = true;
    buf++;
    len--;
  }

  uint64_t unum = 0;
  if (!parse_bytes_uint64_t(buf, len, &unum)) return false;

  int64_t num;
  if (neg) {
    if (unum > (1ull<<63)) return false; // overflow
    num = -(int64_t)unum;
  } else {
    if (unum >= (1ull<<63)) return false; // overflow
    num = (int64_t)unum;
  }

  *_num = num;
  return true;
}

static inline bool parse_bytes_int32_t(const char *buf, size_t len, int32_t *_num)
{
  int64_t num;
  if (!parse_bytes_int64_t(buf, len, &num)) return false;
  if ((int64_t)(int32_t)num != num) return false;
  *_num = (int32_t)num;
  return true;
}

static inline bool parse_bytes_int16_t(const char *buf, size_t len, int16_t *_num)
{
  int64_t num;
  if (!parse_bytes_int64_t(buf, len, &num)) return false;
  if ((int64_t)(int16_t)num != num) return false;
  *_num = (int16_t)num;
  return true;
}

static inline bool parse_bytes_int8_t(const char *buf, size_t len, int8_t *_num)
{
  int64_t num;
  if (!parse_bytes_int64_t(buf, len, &num)) return false;
  if ((int64_t)(int8_t)num != num) return false;
  *_num = (int8_t)num;
  return true;
}
