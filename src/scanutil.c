/*
 * scanutil.c
 * Tim Parys (11/21/2004)
 *
 * This file contains some related scanner functions
 * which are not necessary for the operation of the
 * scanner. Most of these are utility/wrapper functions
 * placed here to keep scan.c a bit neater.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scan.h"

/*
 * push_token
 *    utility function to take the a pointer to a Token struct
 * and then create a cloned copy of it to use later. Memory
 * passed to this function IS NOT claimed, local variables are
 * safe to pass.
 *
 * returns 0 on sucess, nonzero on failure
 */
int push_token(struct Token *inToken, struct ScanData *data) {
  struct StackNode *newnode;
  
  /* allocate space for this token on the stack */
  newnode = MALLOC(struct StackNode);

  /* check for failed allocation */
  if (newnode == NULL) {
    /* allocation failure */
    return -1;
  }
  
  /* otherwise clone input node, and move onto stack */
  memcpy((char*)(&(newnode->token)),(char*)inToken,sizeof(struct Token));
  newnode->next = data->tokBuf;
  data->tokBuf = newnode;
  
  /* success */
  return 0;
}

/*
 * get_token
 *    wrapper function to call the main scanner routine, but use
 * any and all buffered tokens first (this should allow the parser
 * to push a set of tokens back, and allow back-tracking).
 *
 * returns type of token returned
 */
TokenType get_token(struct Token *inToken, struct ScanData *data) {
  struct StackNode *newnode;
  
  /* get top of stack */
  newnode = data->tokBuf;
  
  /* check empty stack */
  if (newnode == NULL) {
    /* empty stack, try scanning instead */
    return scan_token(inToken, data);
  }
  
  /* otherwise pull token and copy contents */
  memcpy((char*)inToken,(char*)(&(newnode->token)),sizeof(struct Token));
  data->tokBuf = newnode->next;
  free(newnode);	/* delete this node */

  /* success */
  return inToken->type;
}

/*
 * peek_token
 *    utility function to find the type of the next token that
 * would be returned by the scanner
 *
 * returns type of token to be read next
 */
TokenType peek_token(struct ScanData *data) {
  struct Token myToken;
  
  /* if the token is on the stack, this is easy */
  if (data->tokBuf != NULL) {
    return data->tokBuf->token.type;
  }

  /* else get a copy of the next token */
  (void)get_token(&myToken,data);
  if (push_token(&myToken, data) != 0) {
    /* cannot push back to stack, very bad */
    fprintf(stderr,"Cannot push token back into buffer\n");
    exit(1);
  }
  
  /* return its type */
  return myToken.type;
}

void clear_token_buffer(struct StackNode *data) {
  struct StackNode *temp;
  while (data != NULL) {
    temp = data->next;
    free(data);
    data = temp;
  }
}

/* bit of a cheap hack to do this, but oh well */
int chop_token_limits(struct Token *tok) {
  int x, tokLen;
  
  if (tok->type != TOK_IDENT_LIMIT) {
    return -1;
  }
  
  tokLen = strlen(tok->token);
  for (x=0; x<tokLen; x++) {
    if ((!isalnum(tok->token[x])) &&
	(tok->token[x] != '_')) {
      tok->token[x] = '\0';
    }
  }
  
  /* advance to first number */
  x=0;
  while (tok->token[x] != '\0') { x++; }
  while (tok->token[x] == '\0') { x++; }
  
  /* scan first number */
  tok->limLow = (int)strtol(&tok->token[x], (char **)NULL, 0);
  
  /* advance to second number */
  while (tok->token[x] != '\0') { x++; }
  while (tok->token[x] == '\0') { x++; }
  
  /* scan first number */
  tok->limHigh = (int)strtol(&tok->token[x], (char **)NULL, 0);
  tok->type = TOK_IDENT;
  
  return 0;
}

