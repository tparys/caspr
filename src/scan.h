#ifndef SCANNER_H
#define SCANNER_H

#include "global.h"

/* this is the enumerated type of what tokens are recognized */
typedef enum {
  TOK_EOF, TOK_ERROR, TOK_IDENT, TOK_IDENT_LIMIT, TOK_LABEL,
  TOK_DIRECTIVE, TOK_INT, TOK_ENDL, TOK_FORMAT, TOK_LPAREN,
  TOK_RPAREN, TOK_ARITHOP } TokenType;

/* here is the generalized Token structure */
struct Token {
  TokenType type;		/* what type of token this is */
  char token[MAX_TOKLEN];	/* the actual token string */
  int value;			/* relevant for integers only */
  int linenum;			/* where the token is in the input */
  int limLow;			/* lower end of bit limit */
  int limHigh;			/* higher end of bit limit */
};

/* linked list stack of tokens for utility functions */
struct StackNode {
  struct Token token;
  struct StackNode *next;
};

/* stuff for scanner, for multiple instances */
struct ScanData {
  FILE *input;
  unsigned int linecount;
  struct StackNode *tokBuf;
};

#define SCANNER_INIT(ptr, handle) {(ptr)->input=handle; (ptr)->linecount = 1; (ptr)->tokBuf=NULL;}
#define SCANNER_STOP(ptr) {clear_token_buffer((ptr)->tokBuf);}
  
/* actual scanner function (unbuffered) */
TokenType scan_token(struct Token *inToken, struct ScanData *data);

/* utility/wrapper functions for scanner */
TokenType get_token(struct Token *inToken, struct ScanData *data);
int push_token(struct Token *inToken, struct ScanData *data);
TokenType peek_token(struct ScanData *data);
void clear_token_buffer(struct StackNode *data);
int chop_token_limits(struct Token *tok);

#endif
