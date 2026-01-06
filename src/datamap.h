#pragma once
#include "header.h"

enum class datamap_type_e : uint8_t
{
  DATAMAP_TYPE_U8 = 0,
  DATAMAP_TYPE_U16,
};

struct datamap_entry_t
{
  char * name;
  datamap_type_e type; /* DATAMAP_TYPE_ */
  uint16_t    addr;
};

struct datamap_t
{
  datamap_entry_t * entries;
  size_t n_entries;
};

datamap_t *datamap_load_from_mem(const char *str, size_t n);
datamap_t *datamap_load_from_file(const char *filename);
void datamap_delete(datamap_t *d);
