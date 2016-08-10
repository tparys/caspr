#ifndef DIRECTIVE_H
#define DIRECTIVE_H

#include "global.h"

/*
 * prototypes
 */

int directive_parse(struct ScanData *scanInfo,
		    struct Token *dirToken,
		    struct SymTab **curSyms,
		    struct ASMRecord **asmrec,
		    unsigned int *offset);

#endif


