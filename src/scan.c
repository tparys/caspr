#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "scan.h"

/* actions to take on the current buffered character */
typedef enum
  { SAVE, TRASH, RETURN }
ChAction;

/* states in state machine */
typedef enum
  { START, COMMENT, IDENT, IDLIMIT1, IDLIMIT2, NUMER_DEC,
    NUMER_OCT, NUMER_HEX, FORMAT, SUBFORMAT, DONE }
StateType;

/*
 * Primary Scanner Function - scan_token
 *
 * Arguments:
 *   token  - a pointer to a token (scanner will write here)
 *
 * Returns:
 *   a enumerated TokenType value, indicating what type of token
 * was written to the token pointer (IDENT, INT, ASSIGN, etc..)
 */
TokenType scan_token(struct Token *inToken, struct ScanData *data) {
  
  /* local function variables */
  StateType curState;		/* current state machine status/state */
  ChAction chStatus;		/* what to do with this character */
  int ch;			/* current character */
  int tIdx;			/* position in token */
  
  /* sanity check for NULL pointers */
  if ((inToken == NULL) || (data->input == NULL)) {
    return TOK_ERROR;
  }
  
  /* initialize scanner token and state machine */
  inToken->type = TOK_EOF;	/* this will be an EOF if nothing read */
  tIdx = 0;			/* position in string token */
  curState = START;		/* start state machine */
  
  /* set token bit limits */
  inToken->limLow = 0;
  inToken->limHigh = 8*sizeof(inToken->value)-1;
  
  /*
   * Main Scanner Loop (state machine)
   *   [ implementation of discrete finite state autonoma ]
   *
   * loop until machine is in DONE state or, scanner is out
   * of token buffer space.
   */
  while ((curState != DONE) && (tIdx < (MAX_TOKLEN - 1)))  {
    
    /* grab next character if able */
    ch = fgetc(data->input);		/* read next character */
    chStatus = SAVE;		/* set save status on character */
    
    /* if uppercase, switch to lower */
    if (isupper(ch)) {
      ch = tolower(ch);
    }
    
    /* if we hit EOF, set DONE state, and kick out */
    if (ch == EOF) {
      curState = DONE;
      continue;
    }

    /* traverse states */
    switch (curState) {

    case START:			/* initial state */

      /* decide what sort of character this is */
      if (isspace(ch) && (ch != '\n')) {
	/* this is whitespace, throw it away */
	chStatus = TRASH;
	curState = START;
      }
      else if (isalpha(ch)) {
	/* alphabetic character - start of identifier */
	inToken->type = TOK_IDENT;
	curState = IDENT;
      }
      else if (ch == '0') {
	/* starts with zero, octal number */
	inToken->type = TOK_INT;
	curState = NUMER_OCT;
      }	
      else if (isdigit(ch)) {
	/* numerical value - start of integer literal */
	inToken->type = TOK_INT;
	curState = NUMER_DEC;
      }
      
      /*
       * the big multicharacter tests didn't claim this character,
       * so use a switch to determine what sort of character this is
       */
      else switch(ch) {
      case '.':
	/* period - start of a assembly directive */
	inToken->type = TOK_DIRECTIVE;
	curState = IDENT;	/* piggy back off identifier logic */
	break;
      case ';':
	/* semicolon, comment char */
	curState = COMMENT;
	break;
      case '{':
	/* left brace, start of format descriptor */
	inToken->type = TOK_ERROR;	/* only until it finishes */
	chStatus = TRASH;
	curState = FORMAT;
	break;
      case '\n':
	/* newline (next line) */
	inToken->type = TOK_ENDL;
	curState = DONE;
	data->linecount += 1;
	break;
      case '$':
	/* specifier for a hex number,
	 * ignore the '$', but replace it with a '0x' */
	chStatus = TRASH;
	curState = START;
	ungetc('x',data->input);	/* tricky tricky */
	ungetc('0',data->input);
	break;
	
      default:
	/* unhandled character */
	inToken->type = TOK_ERROR;
	curState = DONE;
	break;
      }
      break;

    case COMMENT:
      /* comment state, stay here until line terminator */
      if ((ch == '\n') || (ch == '\r')) {
	/* end of comment */
	curState = START;
	tIdx = 0;
	chStatus = RETURN;
      }
      else {
	chStatus = TRASH;		/* throw characters away */
      }
      break;

    case IDENT:
      /* part of an identifier, or possibly an assembly directive */
      /* stay here reading alphabetic, numeric, and '_' characters */
      if (ch == ':') {
	/* ends up being a label */
	inToken->type = TOK_LABEL;
	chStatus = TRASH;
	curState = DONE;
      }
      else if (ch == '<') {
	/* beginning of identifier with a bit limit attached */
	inToken->type = TOK_ERROR;	/* until completed */
	curState = IDLIMIT1;
      }
      else if ((!isalnum(ch)) && (ch != '_')) {
	/* not alphabetic, numeric, or a '_' */
	chStatus = RETURN;	/* push back into stream */
	curState = DONE;	/* we're done with this token */
      }
      break;
      
    case IDLIMIT1:
      /* part of an identifier with bit limit */
      if (ch == '-') {
	/* beginning of second part */
	curState = IDLIMIT2;
      }
      else if (!isdigit(ch)) {
	/* numbers or colons only here, bomb */
	curState = DONE;
      }
      break;
      
    case IDLIMIT2:
      /* part of an identifier with bit limit */
      if (ch == '>') {
	/* end of second part */
	inToken->type = TOK_IDENT_LIMIT;
	curState = DONE;
      }
      else if (!isdigit(ch)) {
	/* numbers or colons only here, bomb */
	curState = DONE;
      }
      break;
      
    case NUMER_DEC:
      /* part of an decimal integer literal */
      /* stay here while reading numeric digit characters */
      if (!isdigit(ch)) {
	/* not a numeric digit */
	chStatus = RETURN;		/* push back into stream */
	curState = DONE;		/* we're done with this token */
      }
      break;
      
    case NUMER_OCT:
      /* part of an octal integer literal */
      /* stay here while reading numeric digit characters */
      if (ch == 'x') {
	/* hexadecimal number */
	curState = NUMER_HEX;
      }
      else if (isdigit(ch)) {
	if (ch > '7') {
	  /* outside of octal range */
	  inToken->type = TOK_ERROR;
	}
      }
      else {
	/* not a numeric digit */
	chStatus = RETURN;		/* push back into stream */
	curState = DONE;		/* we're done with this token */
      }
      break;
      
    case NUMER_HEX:
      /* part of an hexadecimal integer literal */
      /* stay here while reading numeric digit characters */
      if (!isxdigit(ch)) {
	/* not a numeric digit */
	chStatus = RETURN;		/* push back into stream */
	curState = DONE;		/* we're done with this token */
      }
      break;
      
    case FORMAT:
      /* part of a { ... } format descriptor */
      if (ch == '\n') {
	/* may hit one inside format, tick the counter */
	chStatus = TRASH;
	data->linecount += 1;
      }
      else if (isspace(ch)) {
	/* other whitespace .. ignore it and move on */
	chStatus = TRASH;
      } 
      else if ((ch == '0') || (ch == '1')) {
	/* binary bits will be added the returned packet */
      }
      else if (ch == '(') {
	/* subfield start - ie "00 (2) 00" */
	curState = SUBFORMAT;
      }
      else if (ch == '}') {
	/* end of format descriptor */
	inToken->type = TOK_FORMAT;
	chStatus = TRASH;
	curState = DONE;
      }
      else {
	/* unknown character in expression */
	curState = DONE;
      }
      break;
      
    case SUBFORMAT:
      /* part of a { ... } format descriptor */
      if (ch == '\n') {
	/* may hit one inside format, tick the counter */
	chStatus = TRASH;
	data->linecount += 1;
      }
      else if (isspace(ch)) {
	/* other whitespace .. ignore it and move on */
	chStatus = TRASH;
      } 
      else if (isdigit(ch)) {
	/* decimal digits will be added to the returned packet */
      }
      else if (ch == ')') {
	/* subfield end - ie "00 (2) 00" */
	/* go back and finish the format */
	curState = FORMAT;
      }
      else {
	/* unknown character in expression */
	curState = DONE;
      }
      break;
      
    default:
      /* should always have a defined state, but just in case */
      fprintf(stderr, "Invalid State in State Machine\n");
      inToken->type = TOK_ERROR;
      curState = DONE;
      break;
    }

    /* current state processed, what did it say to do? */
    if (chStatus == SAVE) {
      /* save character to token */
      inToken->token[tIdx] = (char)ch;	/* load into token */
      tIdx += 1;				/* increment index */
    }
    else if (chStatus == RETURN) {
      /* return character to stream */
      if (ungetc(ch,data->input) == EOF) {
	/* ungetc failed, this is extremely bad */
	fprintf(stderr,"Cannot push back to stream\n");
	exit(1);
      }
    }
    /* if chStatus == TRASH, then this character is ignored */
  }
  
  /* main loop terminates right above here, so once here, state
     machine is done, and the token is almost formed */
  
  /* state machine is done, so terminate this token string */
  inToken->token[tIdx] = '\0';
  
  /* set line for this token */
  inToken->linenum = data->linecount;
  
  /* if this is an numerical integer, calculate its equivalent value */
  if (inToken->type == TOK_INT) {
    /* stdio to scan this for an integer value */
    //sscanf(inToken->token,"%d",&(inToken->value));
    inToken->value = (int)strtol(inToken->token, (char **)NULL, 0);
  }
  else {
    inToken->value = -1;
  }
  
  /* do bit limits if needed */
  if (inToken->type == TOK_IDENT_LIMIT) {
    chop_token_limits(inToken);
  }
  
  /* return this token type */
  return inToken->type;
}
