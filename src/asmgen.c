#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"
#include "directive.h"

int asmgen_parse_syms(struct SymTab **curSyms,
		      FILE *handle) {
  struct ScanData asmScan;
  unsigned int offset = 0;
  struct Token curToken;
  TokenType ttype;
  struct ASMRecord *rec, *asmrec = NULL;
  int found;
  
  /* set up the scanner */
  SCANNER_INIT(&asmScan,handle);
  
  /* main loop */
  while (1) {
    switch (get_token(&curToken, &asmScan)) {
      
    case TOK_EOF:
      /* end of file */
      SCANNER_STOP(&asmScan);
      
      /* make a special symbol to note size of assembled file */
      symtab_record(curSyms, "$filesize", NULL, offset);
      return 0;
      break;
      
    case TOK_LABEL:
      /* hit a line label */
      symtab_record(curSyms, curToken.token, NULL, offset);
      break;
      
    case TOK_ENDL:
      /* end of line at beginning, skip to next */
      break;
      
    case TOK_DIRECTIVE:
      /* directive, pass current data to directive handler */
      directive_parse(&asmScan, &curToken, curSyms, &asmrec, &offset);
      break;
      
    case TOK_IDENT:
      /* assume to be an assembly mnemonic */
      DEBUG(3) printf("Got identifier - %s\n", curToken.token);
      
      /* find mnemonic in record */
      found = 0;
      for (rec=asmrec; (found==0)&&(rec!=NULL); rec=rec->next) {
	if (strcmp(curToken.token, rec->mnemonic) == 0) {
	  found = 1;
	  offset += rec->byte_count;
	  /* scan tokens until end of line */
	  do {
	    ttype = get_token(&curToken, &asmScan);
	  } while ((ttype != TOK_EOF) && (ttype != TOK_ENDL));
	}
      }
      if (found == 0) {
	fprintf(stderr, "ERROR - mnemonic %s not found\n", curToken.token);
	SCANNER_STOP(&asmScan);
	return -1;
      }
      break;
      
    default:
      fprintf(stderr, "Unexpected Token %s, line %d\n",
	      curToken.token, curToken.linenum);
      SCANNER_STOP(&asmScan);
      return -1;
      break;
    }
  }
}

int asmgen_assemble(struct SymTab **curSyms,
		    FILE *input,
		    unsigned char *data) {
  struct ScanData cfgScan;
  struct Token curToken;
  struct ASMRecord *instr;
  struct ASMRecord *asmcfg = NULL;
  unsigned int argCount, fieldNum, value;
  unsigned int offset = 0, outBits;
  int x;
  
  /* set up the scanner */
  SCANNER_INIT(&cfgScan, input);
  
  /* try to assemble this thing */
  while (1) {
    switch (get_token(&curToken, &cfgScan)) {
      
    case TOK_EOF:
      /* end of file, stop assembling */
      return 0;
      break;
      
    case TOK_LABEL:
    case TOK_ENDL:
      /* ignore these tokens, just pass over */
      break;
      
    case TOK_DIRECTIVE:
      /* directive, pass current data to directive handler */
      directive_parse(&cfgScan, &curToken, curSyms, &asmcfg, &offset);
      break;
      
    case TOK_IDENT:
      /* assume this is a format entry */
      DEBUG(1) printf("\nAssembling mnemonic %s\n", curToken.token);
      
      /* find instruction layout */
      for (instr = asmcfg; (instr!=NULL) &&
	     (strcmp(instr->mnemonic, curToken.token) != 0);
	     instr = instr->next) { }
      
      /* shouldn't happen, but just in case */
      if (instr == NULL) {
	printf("ERROR - Unexpected instruction %s\n", curToken.token);
	return -1;
      }
      
      /* got it, so start assembling */
      DEBUG(1) printf("Found format for instruction %s, %d bytes\n",
		      curToken.token, instr->byte_count);

      outBits = instr->asm_mask;
      for (argCount=0; argCount<instr->num_args; argCount++) {
	/* verify type of next token */
	switch (get_token(&curToken, &cfgScan)) {
	case TOK_INT:
	  /* integer value, try to plug it in */
	  DEBUG(1) printf("Argument %d - integer, value %02x\n", argCount, curToken.value);
	  value = (unsigned int)curToken.value;
	  if (curToken.limHigh - curToken.limLow < 8*sizeof(value) - 1) {
	    value = GETBITS(curToken.limLow, curToken.limHigh, value);
	  }
	  if (CHECK_FIELD_TOO_SMALL(instr->arg_widths[argCount], value)) {
	    printf("WARNING - Value 0x%x not representable with %d bits, line %d\n",
		   value, instr->arg_widths[argCount], curToken.linenum);
	  }
	  break;
	  
	case TOK_IDENT:
	  /* identifier, locate it in the symbol table first */
	  DEBUG(1) printf("Argument %d - identifier\n", argCount);
	  if (symtab_lookup(curSyms, curToken.token, NULL, &value) != 0) {
	    fprintf(stderr, "ERROR - Unknown symbol %s, line %d\n",
		    curToken.token, curToken.linenum);
	    return -1;
	  }
	  if (curToken.limHigh - curToken.limLow < 8*sizeof(value) - 1) {
	    value = GETBITS(curToken.limLow, curToken.limHigh, value);
	  }
	  if (CHECK_FIELD_TOO_SMALL(instr->arg_widths[argCount], value)) {
	    printf("WARNING - Token \'%s\'(0x%x) not representable with %d bits, line %d\n",
		   curToken.token, value, instr->arg_widths[argCount], curToken.linenum);
	  }
	  break;
	  
	default:
	  fprintf(stderr, "ERROR - Argument %d bad, line %d\n",
		  argCount, curToken.linenum);
	  return -1;
	  break;
	}
	
	/* token OK, fill in all fields using this */
	for (fieldNum=0; instr->fmt_args[fieldNum].argNum > -1; fieldNum++) {
	  if (argCount == instr->fmt_args[fieldNum].argNum) {
	    DEBUG(1) printf("Field number %d uses arg %d (value 0x%x)\n",
			    fieldNum, argCount, value);
	    outBits |= GETBITS(0,instr->arg_widths[argCount], value)
	      << instr->fmt_args[fieldNum].argOffset;
	  }
	}
      }
      
      /* expect the newline at the end */
      if (get_token(&curToken, &cfgScan) != TOK_ENDL) {
	fprintf(stderr, "ERROR - Bad token %s at end of line %d\n",
		curToken.token, curToken.linenum);
	return -1;
      }
      
      /* fill in the assembled instruction into the data buffer */
      DEBUG(1) printf("Outputting %d bytes\n", instr->byte_count);
      for (x=(instr->byte_count-1); x>=0; x--) {
	data[offset] = GETBITS(8*x,(8*x)+7, outBits);
	DEBUG(1) printf("Assembled %02x\n", data[offset]);
	offset += 1;
      }
      
      break;
      
    default:
      /* dunno, this is bad */
      fprintf(stderr, "ERROR - Bad token %s at line %d\n",
		curToken.token, curToken.linenum);
	return -1;
      break;
    }
  }
  
  return 0;
}
