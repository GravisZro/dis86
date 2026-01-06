#include "config.h"
#include "dis86_private.h"
#include "bsl/bsl.h"

dis86_decompile_config_t * config_default_new(void)
{
  dis86_decompile_config_t * cfg = (dis86_decompile_config_t*)calloc(1, sizeof(dis86_decompile_config_t));
  return cfg;
}

dis86_decompile_config_t * config_read_new(const char *path)
{
  dis86_decompile_config_t * cfg = (dis86_decompile_config_t*)calloc(1, sizeof(dis86_decompile_config_t));

  size_t sz;
  char * data = read_file(path, &sz);
  if (!data) FAIL("Failed to read file: '%s'", path);

  bsl_t *root = bsl_parse_new(data, sz, NULL);
  if (!root) FAIL("Failed to read the config");

  bsl_t *func = bsl_get_node(root, "dis86.functions");
  if (!func) FAIL("Failed to get functions node");

  bsl_iter_t   it[1];
  int          type;
  const char * key;
  void *       val;

  bsl_iter_begin(it, func);
  while (bsl_iter_next(it, &type, &key, &val)) {
    if (type != BSL_TYPE_NODE) FAIL("Expected function properties");
    bsl_t *f = (bsl_t*)val;

    const char *addr_str = bsl_get_str(f, "start");
    if (!addr_str) FAIL("No function addr property for '%s'", key);

    const char *ret_str = bsl_get_str(f, "ret");
    if (!ret_str) FAIL("No function ret property for '%s'", key);

    const char *args_str = bsl_get_str(f, "args");
    if (!args_str) FAIL("No function args property for '%s'", key);

    bool pop_args_after_call = !bsl_get_str(f, "dont_pop_args");

    int16_t args;
    if (!parse_bytes_int16_t(args_str, strlen(args_str), &args)) FAIL("Expected uint16_t for '%s.args', got '%s'", key, args_str);

    assert(cfg->func_len < ARRAY_SIZE(cfg->func_arr));
    config_func_t *cf = &cfg->func_arr[cfg->func_len++];
    cf->name = strdup(key);
    cf->addr = parse_segoff(addr_str);
    cf->ret  = strdup(ret_str);
    cf->args = args;
    cf->pop_args_after_call = pop_args_after_call;
  }

  bsl_t *glob = bsl_get_node(root, "dis86.globals");
  if (!glob) FAIL("Failed to get globals node");

  bsl_iter_begin(it, glob);
  while (bsl_iter_next(it, &type, &key, &val)) {
    if (type != BSL_TYPE_NODE) FAIL("Expected global properties");
    bsl_t *f = (bsl_t*)val;

    const char *off_str = bsl_get_str(f, "off");
    if (!off_str) FAIL("No global off property for '%s'", key);

    const char *type_str = bsl_get_str(f, "type");
    if (!type_str) FAIL("No global type property for '%s'", key);

    assert(cfg->global_len < ARRAY_SIZE(cfg->global_arr));
    config_global_t *g = &cfg->global_arr[cfg->global_len++];
    g->name   = strdup(key);
    g->offset = parse_hex_uint16_t(off_str, strlen(off_str));
    g->type   = strdup(type_str);
  }

  bsl_t *segmap = bsl_get_node(root, "dis86.segmap");
  if (!segmap) FAIL("Failed to get segmap node");

  bsl_iter_begin(it, segmap);
  while (bsl_iter_next(it, &type, &key, &val)) {
    if (type != BSL_TYPE_NODE) FAIL("Expected segmap properties");
    bsl_t *s = (bsl_t*)val;

    const char *from_str = bsl_get_str(s, "from");
    if (!from_str) FAIL("No segmap 'from' property for '%s'", key);
    uint16_t from = parse_hex_uint16_t(from_str, strlen(from_str));

    const char *to_str = bsl_get_str(s, "to");
    if (!to_str) FAIL("No segmap 'to' property for '%s'", key);
    uint16_t to = parse_hex_uint16_t(to_str, strlen(to_str));

    assert(cfg->segmap_len < ARRAY_SIZE(cfg->segmap_arr));
    config_segmap_t *sm = &cfg->segmap_arr[cfg->segmap_len++];
    sm->name = strdup(key);
    sm->from = from;
    sm->to = to;
  }


  bsl_delete(root);
  free(data);

  return cfg;
}

void config_delete(dis86_decompile_config_t *cfg)
{
  if (!cfg) return;
  for (size_t i = 0; i < cfg->func_len; i++) {
    free(cfg->func_arr[i].name);
  }
  for (size_t i = 0; i < cfg->global_len; i++) {
    free(cfg->global_arr[i].name);
    free(cfg->global_arr[i].type);
  }
  for (size_t i = 0; i < cfg->segmap_len; i++) {
    free(cfg->segmap_arr[i].name);
  }
  free(cfg);
}

void config_print(dis86_decompile_config_t *cfg)
{
  printf("functions:\n");
  for (size_t i = 0; i < cfg->func_len; i++) {
    config_func_t *f = &cfg->func_arr[i];
    printf("  %-30s  %04x:%04x  %-8s  %d\n", f->name, f->addr.seg, f->addr.off, f->ret, f->args);
  }
  printf("\n");
  printf("globals:\n");
  for (size_t i = 0; i < cfg->global_len; i++) {
    config_global_t *g = &cfg->global_arr[i];
    printf("  %-30s  %04x  %s\n", g->name, g->offset, g->type);
  }
  printf("\n");
  printf("segmap:\n");
  for (size_t i = 0; i < cfg->segmap_len; i++) {
    config_segmap_t *s = &cfg->segmap_arr[i];
    printf("  %-30s  %04x => %04x\n", s->name, s->from, s->to);
  }
}

config_func_t * config_func_lookup(dis86_decompile_config_t *cfg, segoff_t s)
{
  for (size_t i = 0; i < cfg->func_len; i++) {
    config_func_t *f = &cfg->func_arr[i];
    if (f->addr.seg == s.seg && f->addr.off == s.off) {
      return f;
    }
  }
  return NULL;
}

bool config_seg_remap(dis86_decompile_config_t *cfg, uint16_t *_seg)
{
  uint16_t seg = *_seg;
  for (size_t i = 0; i < cfg->segmap_len; i++) {
    config_segmap_t *sm = &cfg->segmap_arr[i];
    if (seg == sm->from) {
      *_seg = sm->to;
      return true;
    }
  }
  return false;
}

dis86_decompile_config_t * dis86_decompile_config_read_new(const char *path)
{ return config_read_new(path); }

void dis86_decompile_config_delete(dis86_decompile_config_t *cfg)
{ config_delete(cfg); }
