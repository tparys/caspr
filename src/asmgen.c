#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"
#include "directive.h"

int asmgen_parse_value(struct ScanData *scanner,
		       struct SymTab **curSyms,
		       unsigned int *pResult) {
  struct Token curToken;
  unsigned int lval, rval;
  
  /* scan next token or expression */
  switch (get_token(&curToken, scanner)) {
    
  case TOK_INT:
    /* integer value, just pass it back */
    DEBUG(1) printf("Got an integer - %d\n", curToken.value);
    lval = curToken.value;
    if (curToken.limHigh - curToken.limLow < 8*sizeof(lval) - 1) {
      /* if token is specified with a bitslice, use only those bits */
      lval = GETBITS(curToken.limLow, curToken.limHigh, lval);
    }
    *pResult = lval;
    return 0;
    break;
    
  case TOK_IDENT:
    /* identifier, locate it in the symbol table */
    DEBUG(1) printf("Got a symbol lookup - '%s'\n", curToken.token);
    if (symtab_lookup(curSyms, curToken.token, NULL, (int*)&lval) == 0) {
      /* symbol table lookup success */
      if (curToken.limHigh - curToken.limLow < 8*sizeof(lval) - 1) {
	/* if token is specified with a bitslice, use only those bits */
	lval = GETBITS(curToken.limLow, curToken.limHigh, lval);
      }
      *pResult = lval;
      return 0;
    }
    printf("ERROR - Symbol '%s' Not Found\n", curToken.token);
    break;
    
  case TOK_LPAREN:
    /* left parentheses, beginning of an arithmetic expression */
    
    /* start by evaluating first term (may be another expression */
    if (asmgen_parse_value(scanner, curSyms, &lval) != 0) {
      /* error parsing sub-expression */
      printf("ERROR - Cannot Parse Arithmetic Expression\n");
      return -1;
    }
    
    /* inner loop to evalate left-to-right arithmetic expressions */
    while (1) {
      /* get next token */
      switch (get_token(&curToken, scanner)) {
	
      case TOK_RPAREN:
	/* close parentheses, end of sub-expression */
	*pResult = lval;
	return 0;
	break;
	
      case TOK_ARITHOP:
	/* arithmetic operation (+/-), get right hand value */
	if (asmgen_parse_value(scanner, curSyms, &rval) != 0) {
	  /* error parsing sub-expression */
	  printf("ERROR - Cannot Parse Arithmetic Expression\n");
	  return -1;
	}
	
	/* handle arithmetic operation */
	switch (curToken.token[0]) {
	case '+':
	  lval += rval;
	  break;
	case '-':
	  lval -= rval;
	  break;
	default:
	  /* unhandled */
	  printf("ERROR - Unknown Arithmetic operation '%s'\n", curToken.token);
	  return -1;
	  break;
	}
	break;
	
      default:
	printf("ERROR - Unexpected token '%s' while parsing subexpression\n",
	       curToken.token);
	return -1;
	break;
      }
    }
    break;
    
  default:
    /* unknown token */
    printf("Unhandled token \'%s\'\n", curToken.token);
    break;
  }
  return -1;
}

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
		    char *data) {
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
      directive_parse(&cfgScan, &curToken, NULL, &asmcfg, &offset);
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
	/* parse next token or parenthesized expression */
	if (asmgen_parse_value(&cfgScan, curSyms, &value) != 0) {
	  fprintf(stderr, "ERROR - Argument %d bad, line %d\n",
		  argCount, curToken.linenum);
	  return -1;
	  }
	if (CHECK_FIELD_TOO_SMALL(instr->arg_widths[argCount], value)) {
	  printf("WARNING - Value 0x%x not representable with %d bits, line %d\n",
		 value, instr->arg_widths[argCount], curToken.linenum);
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
