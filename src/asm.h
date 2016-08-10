#ifndef ASMGEN_H
#define ASMGEN_H

#include "global.h"
#include "scan.h"
#include "symtab.h"

/*
 * defines
 */

/* 90% of assembly instructions shouldn't have more */
#define MAX_ASM_ARGS 16

/* defines to ease life some */
#define ASMREC_ARGNUM(ptr, fmtarg) ((ptr)->fmt_args[fmtarg].argNum)
#define ASMREC_OFFSET(ptr, fmtarg) ((ptr)->fmt_args[fmtarg].argOffset)
#define ASMREC_WIDTH(ptr, fmtarg) ((ptr)->arg_widths[(ptr)->fmt_args[fmtarg].argNum])

/*
 * data structures
 */

/* holds information for each field specifier
 * in instuction configuration format */
struct ArgFormat {
  int8_t argNum;	/* which argument it uses */
  int8_t argOffset;	/* how much offset inside the asm */
};

/* holds information for each assembly mnemonic */
struct ASMRecord {
  struct ASMRecord *next;		     /* linked list */
  char             mnemonic[MAX_TOKLEN];     /* instruction name */
  uint32_t         asm_mask;                 /* instruction with all fields 0 */
  uint8_t          arg_widths[MAX_ASM_ARGS];
  uint8_t          byte_count;               /* number of bytes */
  uint8_t          num_args;                 /* number fields to fill */
  struct ArgFormat fmt_args[MAX_ASM_ARGS];   /* info for each field to fill */
};

/*
 * prototypes
 */

/* loading configuration records for a given instruction set */
struct ASMRecord* asmrec_load(struct SymTab **curSyms, char *infile);
int asmrec_free(struct ASMRecord *ptr);

/* generation of machine code */
int asmgen_parse_value(struct ScanData *scanner,
		       struct SymTab **curSyms,
		       unsigned int *pResult);
int asmgen_parse_syms(struct SymTab **curSyms,
		      FILE *handle);
int asmgen_assemble(struct SymTab **curSyms,
		    FILE *input,
		    char *data);

/* file output */
int asmout_make_rom(struct SymTab **curSyms, char *out, char *data);
int asmout_make_mif(struct SymTab **curSyms, char *out, char *data);

#endif
