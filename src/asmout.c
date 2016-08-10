#include <stdio.h>
#include "asm.h"

int asmout_make_mif(struct SymTab **curSyms, char *out, char *data) {
  FILE *handle;
  int x, t, datasize, mifwords, mifbytes;
  int bytewidth, mifwidth = 8;
  
  /* sanity check */
  if (symtab_lookup(curSyms, "$filesize", NULL, &datasize) != 0) {
    fprintf(stderr, "ERROR - Unknown file size\n");
    return -1;
  }
  if (symtab_lookup(curSyms, "$mifwords", NULL, &mifwords) != 0) {
    fprintf(stderr, "ERROR - Unknown MIF output size\n");
    return -1;
  }
  if (symtab_lookup(curSyms, "$mifwidth", NULL, &mifwidth) == 0) {
    if ((mifwidth == 0) && ((mifwidth % 8) != 0)) {
      fprintf(stderr, "ERROR - Illegal MIF width size "
	      "(must be multiple of 8)\n");
      return -1;
    }
  }
  
  bytewidth = mifwidth / 8;
  mifbytes = mifwords * bytewidth;
  if (datasize > mifbytes) {
    fprintf(stderr, "ERROR - Assembled file will not fit within "
	    "mif filesize\n");
    return -1;
  }
  
  /* open output */
  if ((handle = fopen(out, "w")) == NULL) {
    perror("ERROR - Could not open output file");
    return -1;
  }
  
  /* dump MIF header */
  fprintf(handle,
	  "-- caspr\n\n"
	  "WIDTH=%d;\n"
	  "DEPTH=%d;\n\n"
	  "ADDRESS_RADIX=HEX;\n"
	  "DATA_RADIX=HEX;\n\n"
	  "CONTENT BEGIN\n",
	  mifwidth, mifwords);
  
  /* output each assembled unit (no optimization for now) */
  for (x=0; x<mifbytes; x+=bytewidth) {
    fprintf(handle, "\t%x  :   ", x / bytewidth);
    for (t=0; t<bytewidth; t++) {
      if ((x+t) < datasize) {
	fprintf(handle, "%02X", data[x+t] & 0xff);
      }
      else {
	fprintf(handle, "00");
      }
    }
    fprintf(handle, ";\n");
  }
  
  /* dump MIF trailer */
  fprintf(handle, "END;\n");
  
  /* done */
  fclose(handle);
  return 0;
}

int asmout_make_rom(struct SymTab **curSyms, char *out, char *data) {
  FILE *handle;
  int x, y, t, datasize;
  
  /* sanity check */
  if (symtab_lookup(curSyms, "$filesize", NULL, &datasize) != 0) {
    fprintf(stderr, "ERROR - Unknown file size\n");
    return -1;
  }

  /* open output */
  if ((handle = fopen(out, "w")) == NULL) {
    perror("ERROR - Could not open output file");
    return -1;
  }
  
  /* dump ROM header (none for now) */


  DEBUG(1) printf("Max Address %d\n", datasize);
  /* output each row */
  for (x=0; x<datasize; x+=8) {
    fprintf(handle, "0x%04X |", x);
    for (y=0; y<8; y++) {
      if ((x+y)<datasize) {
	t = (data[x+y] & 0xff);
	fprintf(handle, " %02X", t);
      }
      else {
	fprintf(handle, " 00");
      }
    }
    fprintf(handle, "\n");
  }
  
  /* dump ROM trailer (none for now) */
  
  /* done */
  fclose(handle);
  return 0;
}
