#ifndef SYMTAB_H
#define SYMTAB_H

#include "global.h"

/* simple linked list to hold symbols (hash table would be better) */
struct SymTab {
  char name[MAX_TOKLEN];
  char strVal[MAX_TOKLEN];
  int intVal;
  struct SymTab *next;
};

/* prototypes */

int symtab_clear(struct SymTab **curSyms);
int symtab_record(struct SymTab **curSyms, char *name, char *strVal, int intVal);
int symtab_lookup(struct SymTab **curSyms, char *name, char *strOut, int *intOut);
int symtab_show(struct SymTab **curSyms);

#endif
