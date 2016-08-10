#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"
#include "directive.h"

static char *cfg_file_formats[3] =
  { "%s.cfg",
    "cfg/%s.cfg",
    NULL };

/* allocate one empty asm record */
int asmrec_init(struct ASMRecord *ptr) {
  int x;
  
  /* check */
  if (ptr == NULL) {
    /* failure */
    return -1;
  }
  
  /* initialize to zero */
  memset((char*)ptr, 0, sizeof(struct ASMRecord));
  
  /* initialize nonzero flags and stuff */
  ptr->byte_count = -1;
  ptr->num_args = 0;
  
  /* set all possible arg fields to unused/invalid status */
  for (x=0; x<MAX_ASM_ARGS; x++) {
    ptr->fmt_args[x].argNum = -1;
    ptr->fmt_args[x].argOffset = -1;
  }
  
  /* done */
  return 0;
}

/* set the width of the next uninitialized argument to the
 * set width. seems a bit overkill, but should make the
 * config file parser a bit easier (may replace with a
 * macro later) */
int asmrec_add_arg(struct ASMRecord *ptr, int width) {
  
  /* sanity check */
  if (ptr == NULL) {
    return -1;
  }
  
  /* add next width in, bump the arg counter */
  ptr->arg_widths[ptr->num_args++] = width;
  
  return 0;
}

int asmrec_parse_format(struct ASMRecord *ptr, char *fmt) {
  int x, i, fmt_count, bitcount, bitinc, argused;
  char buf[MAX_TOKLEN];
  uint32_t imask = 0x00000000;
  
  /* sanity check */
  if (ptr == NULL) {
    return -1;
  }
  
  DEBUG(1) printf("Parsing format \'%s\'\n", fmt);
  
  /* count number of bits in instruction format,
   * and make a mask of the appropriate opcode
   * to OR with the fields later */
  bitcount = 0;
  fmt_count = 0;
  for (x=0; fmt[x]!='\0'; x++) {
    argused = -1;
    bitinc = 0;
    switch (fmt[x]) {
    
    case '0':
      imask = imask << 1;
      bitinc = 1;
      break;
      
    case '1':
      imask = imask << 1;
      imask += 1;
      bitinc = 1;
      break;
      
    case '(':
      /* convert "(X)" into numeric 'X' */
      i = 0;
      memset(buf, 0, MAX_TOKLEN);	/* clear string */
      for (x++; fmt[x]!=')'; x++) {
	buf[i++] = fmt[x];
      }
      
      /* convert this to a numeric value */
      i = (int)strtol(buf, (char **)NULL, 0);
      if ((i >= 0) && (i < ptr->num_args)) {
	ptr->fmt_args[fmt_count].argNum = i;
	imask = imask << ptr->arg_widths[i];
	bitinc = ptr->arg_widths[i];
	argused = fmt_count++; /* save this for a bit */
      }
      else {
	printf("ERROR - Invalid subfield specifier, \"(%d)\"\n", i);
      }
      break;
    }
    
    /* update known offsets */
    bitcount += bitinc;
    for (i=0; i<ptr->num_args; i++) {
      if (ptr->fmt_args[i].argOffset != -1) {
	ptr->fmt_args[i].argOffset += bitinc;
      }
    }
    if (argused != -1) {
      ptr->fmt_args[argused].argOffset = 0;
    }
    
  }
	   
  
  /* check that its multiple of 8 (byte aligned) */
  if ((bitcount % 8) != 0) {
    printf("ERROR - Instruction format is not byte aligned\n");
    return -1;
  }
  ptr->byte_count = bitcount / 8;
  ptr->asm_mask = imask;
  DEBUG(1) printf("Instruction is %d bytes wide\n", ptr->byte_count);
  DEBUG(1) printf("Instruction mask is %04X\n", ptr->asm_mask);
  
  return 0;
}

int asmrec_free(struct ASMRecord *ptr) {
  struct ASMRecord *tmp;
  while (ptr != NULL) {
    tmp = ptr->next;
    free(ptr);
    ptr = tmp;
  }
  return 0;
}

struct ASMRecord* asmrec_load(struct SymTab **curSyms, char *infile) {
  struct ScanData cfgScan;
  struct ASMRecord *entry, *stack = NULL;
  struct Token curToken;
  char filename[256], **fmt;
  FILE *handle = NULL;
  TokenType ttype;		/* type of current token */
  int x;
  
  /* try to open file */
  for (fmt = cfg_file_formats; handle == NULL; fmt = &(fmt[1])) {
    if (*fmt == NULL) {
      DEBUG(2) fprintf(stderr, "Tried to open %s, but could not\n", infile);
      return NULL;
    }
    else {
      sprintf(filename, *fmt, infile);
      DEBUG(2) printf("Trying to open %s\n", filename);
      handle = fopen(filename, "r");
    }
  }
  
  /* file is open */
  DEBUG(2) printf("file open\n");
  SCANNER_INIT(&cfgScan,handle);
  
  while (1) {
    switch (get_token(&curToken, &cfgScan)) {
      
    case TOK_EOF:
      /* end of file */
      SCANNER_STOP(&cfgScan);
      return stack;
      break;
      
    case TOK_ENDL:
      /* ignore this token, pass over it */
      break;
      
    case TOK_DIRECTIVE:
      /* directive, pass current data to directive handler */
      directive_parse(&cfgScan, &curToken, curSyms, NULL, NULL);
      break;
      
    case TOK_IDENT:
      /* assume this is a format entry */
      DEBUG(1) printf("\nGot a format entry for %s\n", curToken.token);
      
      /* set up a record for it */
      entry = MALLOC(struct ASMRecord);
      asmrec_init(entry);
      
      /* set name */
      strncpy(entry->mnemonic, curToken.token, MAX_TOKLEN);
      
      /* add argument widths as needed */
      ttype = scan_token(&curToken, &cfgScan);
      while (ttype == TOK_INT) {
	asmrec_add_arg(entry, curToken.value);
	ttype = get_token(&curToken, &cfgScan);
      }
      
      /* the format should be next */
      if (ttype == TOK_FORMAT) {
	if (asmrec_parse_format(entry, curToken.token) != 0) {
	  /* could not parse, forget it */
	  printf("ERROR - Format unusable for %s, attempting "
		 "to continue without it\n", entry->mnemonic);
	  free(entry);
	}
	else {
	  DEBUG(1) printf("Got format with %d argument fields\n",
			  entry->num_args);
	  for (x=0; x<entry->num_args; x++) {
	    DEBUG(1) printf("-> Arg %d, Width %d, Offset %d\n", 
			    ASMREC_ARGNUM(entry, x),
			    ASMREC_WIDTH(entry, x),
			    ASMREC_OFFSET(entry, x));
	  }
	
	  /* add to linked list */
	  entry->next = stack;
	  stack = entry;
	}
      }
      else {
	printf("FATAL - Unexpected Token %s, line %d\n",
	       curToken.token, curToken.linenum);
	free(entry);
	SCANNER_STOP(&cfgScan);
	asmrec_free(stack);
	return NULL;
      }
      break;
      
    default:
      printf("Unexpected Token %s, line %d\n",
	     curToken.token, curToken.linenum);
      SCANNER_STOP(&cfgScan);
      asmrec_free(stack);
      return NULL;
      break;
    }
  }
  return NULL;
}
