#ifndef DIS86_H
#define DIS86_H

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

#include "decompile/config.h"
#include "common/segment.h"

#include "binary.h"
#include "instr.h"

/*****************************************************************/
/* CORE TYPES */
/*****************************************************************/
struct dis86_t
{
  binary_t b[1];
  dis86_instr_t ins[1];
};


enum {
  RESULT_SUCCESS = 0,
  RESULT_NEED_OPCODE2,
  RESULT_NOT_FOUND,
};


/*****************************************************************/
/* CORE ROUTINES */
/*****************************************************************/

/* Create new instance: deep copies the memory */
dis86_t *dis86_new(size_t base_addr, segment<uint8_t> mem);

/* Destroys an instance */
void dis86_delete(dis86_t *d);

/* Get next instruction */
dis86_instr_t *dis86_next(dis86_t *d);

/* Get Position */
size_t dis86_position(dis86_t *d);

/* Get Baseaddr */
size_t dis86_baseaddr(dis86_t *d);

/* Get Length */
size_t dis86_length(dis86_t *d);

/*****************************************************************/
/* INSTR ROUTINES */
/*****************************************************************/

/* Get the address where the instruction resides */
size_t dis86_instr_addr(dis86_instr_t *ins);

/* Get the number of bytes used in the encoding */
size_t dis86_instr_n_bytes(dis86_instr_t *ins);

/* Copy the instruction */
void dis86_instr_copy(dis86_instr_t *dst, dis86_instr_t *src);

/*****************************************************************/
/* PRINT ROUTINES */
/*****************************************************************/

/* Print */
char *dis86_print_intel_syntax(dis86_t *d, dis86_instr_t *ins, bool with_detail);

/*****************************************************************/
/* DECOMPILE ROUTINES */
/*****************************************************************/


/* Construct a config from file */
dis86_decompile_config_t * dis86_decompile_config_read_new(const char *path);
void                       dis86_decompile_config_delete(dis86_decompile_config_t *cfg);

/* Decompile to C code */
char *dis86_decompile( dis86_t *                  dis,
                       dis86_decompile_config_t * opt_cfg, /* optional */
                       const char *               func_name,
                       uint16_t                   seg,
                       dis86_instr_t *            ins,
                       size_t                     n_ins );


#endif
