#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"
#include "symtab.h"

int guess_output(struct SymTab **prgSyms, char *out) {
  int x, pIdx = -1;
  char fmtname[MAX_TOKLEN];
  
  if (symtab_lookup(prgSyms, "$outfmt", fmtname, NULL) != 0) {
    printf("INFO: Unknown output format, defaulting to mif\n");
    sprintf(fmtname, "mif");
  }
  
  for (x=0; out[x] != '\0'; x++) {
    if (out[x] == '.') {
      pIdx = x;
    }
  }
  
  if (pIdx == -1) {
    return -1;
  }
  
  sprintf(&out[pIdx+1], "%s", fmtname);
  return 0;
}
  
int main(int argc, char **argv) {
  /* local vars */
  struct SymTab *prgSyms = NULL;
  FILE *inFile;
  unsigned char *data, outfmt[64];
  unsigned char *outName, guessed[1024];
  int prgSize, ret;
  
  symtab_clear(&prgSyms);
  
  /* check if there was a file specified as an argument */
  if (argc < 2) {
    printf("No input specified\n\n");
    printf("Usage:\n\t%s <input> [<output>]\n", argv[0]);
    return 0;
  }
  
  /* open input file */
  if ((inFile = fopen(argv[1], "r")) == NULL) {
    perror("FATAL - Could not open input file");
    return -1;
  }
  
  /* load the symbol table from the input file given */
  if (asmgen_parse_syms(&prgSyms, inFile) != 0) {
    /* failed */
    fprintf(stderr, "FATAL - Could not parse input\n");
    symtab_clear(&prgSyms);
    return -1;
  }
  
  /* display known symbols */
  if (0) {
    symtab_show(&prgSyms);
  }
  
  /* allocate space to write binary program data */
  if (symtab_lookup(&prgSyms, "$filesize", NULL, &prgSize) != 0) {
    fprintf(stderr, "ERROR - Unknown file size\n");
    return -1;
  }
  if ((data = CALLOC(unsigned char, prgSize)) == NULL) {
    fprintf(stderr, "ERROR - Memory allocation failed\n");
    return -1;
  }
  
  /* rewind input file to beginning for second pass */
  if (fseek(inFile, 0L, SEEK_SET) != 0) {
    perror("FATAL - Could not rewind file");
    return -1;
  }
  
  /* attempt to assemble */
  if (asmgen_assemble(&prgSyms, inFile, data) != 0) {
    fprintf(stderr, "FATAL - Could not assemble\n");
    return -1;
  }
  
  /* identify output filename */
  if (argc < 3) {
    outName = guessed;
    strncpy(guessed, argv[1], 1024);
    guess_output(&prgSyms, guessed);
  }
  else {
    /* specified, use that */
    outName = argv[2];
  }
  printf("Output name is \'%s\'\n", outName);
  
  /* write file */
  symtab_lookup(&prgSyms, "$outfmt", outfmt, NULL);
  if (strcmp(outfmt, "mif") == 0) {
    ret = asmout_make_mif(&prgSyms, outName, data) != 0;
  }
  else {
    ret = asmout_make_rom(&prgSyms, outName, data) != 0;
  }
  if (ret != 0) {
    fprintf(stderr, "FATAL - File output failed\n");
    return -1;
  }
  
  return 0;
}
