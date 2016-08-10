#ifndef GLOBAL_H
#define GLOBAL_H

#include <inttypes.h>

/* size of internal character buffers */
#define MAX_TOKLEN 64
#define BUFSIZE 512

#define UINT32 unsigned long
#define UINT64 unsigned long long

/* frequently used macros for bit twiddling */
#define GETBIT(x,val)    ((val>>x)&0x01)
#define GETBITS(x,y,val) ((val>>x)&(((UINT32)1<<(y-x+1))-1))
#define GETBITS64(x,y,val) ((val>>x)&(((UINT64)1<<(y-x+1))-1))

/* to save some typing while casting malloc & calloc */
#define MALLOC(x)   (x*)malloc(sizeof(x))
#define CALLOC(x,y) (x*)calloc(sizeof(x),y)

/* set macro to control debug levels */
#define DEBUG_LEVEL 0
#define DEBUG(x) if (DEBUG_LEVEL >= x)

/* check possible values of set width fields */
/* #define CHECK_FIELD_TOO_SMALL(width, value) \ */
/*  (((value) >= (1<<(width))) || ((value) < (0-(1<<(width-1))))) */
#define CHECK_FIELD_TOO_SMALL(width, value) \
  ((value) >= (1<<(width)))

#endif 
