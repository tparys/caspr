#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"
#include "directive.h"

int directive_parse(struct ScanData *scanInfo,
		    struct Token *dirToken,
		    struct SymTab **curSyms,
		    struct ASMRecord **asmrec,
		    unsigned int *offset) {
  char tokName[MAX_TOKLEN];
  struct Token newToken;
  
  /* architecture selection directive */
  if ((strcmp(dirToken->token, ".arch") == 0)) {
    /* next token should be an identifier to our architecture file */
    if (get_token(&newToken, scanInfo) != TOK_IDENT) {
      fprintf(stderr, "ERROR - Unexpected Token %s, line %d\n",
	      newToken.token, newToken.linenum);
      return -1;
    }
    
    /* check that we have a valid pointer to write to */
    if (asmrec != NULL) {
      if (*asmrec != NULL) {
	asmrec_free(*asmrec);
      }
      DEBUG(1) printf("Loading architecture %s\n", newToken.token);
      *asmrec = asmrec_load(curSyms, newToken.token);
      if (*asmrec == NULL) {
	printf("Could not load architecture file for %s\n", newToken.token);
      }
    }
  }
  
  /* define directive */
  else if ((strcmp(dirToken->token, ".define") == 0)) {
    /* next token should be an string to define */
    if (get_token(&newToken, scanInfo) != TOK_IDENT) {
      fprintf(stderr, "ERROR - Unexpected Token %s, line %d\n",
	      newToken.token, newToken.linenum);
      return -1;
    }
    strcpy(tokName, newToken.token);
    
    /* check that we have a valid pointer to write to */
    if (curSyms != NULL) {
      /* next token should be a numeric value to set */
      if (get_token(&newToken, scanInfo) != TOK_INT) {
	fprintf(stderr, "ERROR - Unexpected Token %s, line %d\n",
		newToken.token, newToken.linenum);
	return -1;
      }
      symtab_record(curSyms, tokName, NULL, newToken.value);
    }
  }
  
  /* output specifier define */
  else if ((strcmp(dirToken->token, ".outfmt") == 0)) {
    /* next token should be an string specifying output */
    if (get_token(&newToken, scanInfo) != TOK_IDENT) {
      fprintf(stderr, "ERROR - Unexpected Token %s, line %d\n",
	      newToken.token, newToken.linenum);
      return -1;
    }
    
    /* check that we have a valid pointer to write to */
    if (curSyms != NULL) {
      symtab_record(curSyms, "$outfmt", newToken.token, -1);
    }
  }
  
  /* output position define */
  else if ((strcmp(dirToken->token, ".org") == 0)) {
    /* next token should be an string specifying output */
    if (get_token(&newToken, scanInfo) != TOK_INT) {
      fprintf(stderr, "ERROR - Unexpected Token %s, line %d\n",
	      newToken.token, newToken.linenum);
      return -1;
    }
    
    /* check that we have a valid pointer to write to */
    if (offset != NULL) {
      *offset = newToken.value;
    }
  }
  
  /* output specifier define */
  else if ((strcmp(dirToken->token, ".mifwords") == 0)) {
    /* next token should be an integer specifying size */
    if (get_token(&newToken, scanInfo) != TOK_INT) {
      fprintf(stderr, "ERROR - Unexpected Token %s, line %d\n",
	      newToken.token, newToken.linenum);
      return -1;
    }
    
    /* check that we have a valid pointer to write to */
    if (curSyms != NULL) {
      symtab_record(curSyms, "$mifwords", NULL, newToken.value);
    }
  }
  
  /* output specifier define */
  else if ((strcmp(dirToken->token, ".mifwidth") == 0)) {
    /* next token should be an integer specifying size */
    if (get_token(&newToken, scanInfo) != TOK_INT) {
      fprintf(stderr, "ERROR - Unexpected Token %s, line %d\n",
	      newToken.token, newToken.linenum);
      return -1;
    }
    
    /* check that we have a valid pointer to write to */
    if (curSyms != NULL) {
      symtab_record(curSyms, "$mifwidth", NULL, newToken.value);
    }
  }
  
  /* unknown directive */
  else {
    printf("ERROR - Unknown directive %s\n", dirToken->token);
  }
  
  /* chew tokens until end of line */
  while (get_token(&newToken, scanInfo) != TOK_ENDL) {
    DEBUG(3) printf("-> ignoring token %s\n", newToken.token);
  }
  
  return 0;
}
