#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "scan.h"

int symtab_clear(struct SymTab **curSyms) {
  struct SymTab *temp;
  
  /* sanity check */
  if (curSyms == NULL) {
    return -1;
  }
  
  while (*curSyms != NULL) {
    temp = (*curSyms)->next;
    free(*curSyms);
    *curSyms = temp;
  }
  
  return 0;
}

int symtab_record(struct SymTab **curSyms, char *name, char *strVal, int intVal) {
  struct SymTab *newsym, *loop;
  
  /* sanity check */
  if ((curSyms == NULL) || (name == NULL)){
    return -1;
  }
  
  for (loop = *curSyms; loop != NULL; loop = loop->next) {
    if (strcmp(loop->name, name)==0) {
      /* already exists, overwrite */
      loop->intVal = intVal;
      if (strVal != NULL) {
	strcpy(loop->strVal, strVal);
      }
      else {
	loop->strVal[0] = '\0';
      }
      return 0;
    }
  }
  
  /* not found, record new */
  if ((newsym = MALLOC(struct SymTab)) == NULL) {
    /* allocation error */
    fprintf(stderr, "FATAL - Could not allocate space\n");
    exit(-1);
  }
  
  /* record and add into list */
  strcpy(newsym->name, name);
  newsym->intVal = intVal;
  if (strVal != NULL) {
    strcpy(newsym->strVal, strVal);
  }
  else {
    newsym->strVal[0] = '\0';
  }
  newsym->next = *curSyms;
  *curSyms = newsym;

  return 0;
}

int symtab_lookup(struct SymTab **curSyms, char *name, char *strOut, int *intOut) {
  struct SymTab *loop;
  
  /* sanity check */
  if ((curSyms == NULL) || (name == NULL)){
    return -1;
  }
  
  for (loop = *curSyms; loop != NULL; loop = loop->next) {
    if (strcmp(loop->name, name)==0) {
      if (intOut != NULL) {
	*intOut = loop->intVal;
      }
      if (strOut != NULL) {
	strcpy(strOut, loop->strVal);
      }
      return 0;
    }
  }
  
  /* not found */
  return -1;
}

int symtab_show(struct SymTab **curSyms) {
  struct SymTab *loop = *curSyms;
  
  /* sanity check */
  if (curSyms == NULL){
    return -1;
  }
  
  printf("Listing of known symbols\n");
  for (loop = *curSyms; loop != NULL; loop = loop->next) {
    if (loop->strVal[0] != '\0') {
      printf("-> %-15s%s\n",
	     loop->name, loop->strVal);
    }
    else {
      printf("-> %-15s%d (0x%x)\n",
	     loop->name, loop->intVal, loop->intVal);
    }
  }
  return 0;
}
