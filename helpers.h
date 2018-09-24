#ifndef _HELPERS_H
#define _HELPERS_H

#define MAXLEN	256
#define MISSING_VALUE	"NA"
#define MAX(A, B)         ((A) > (B) ? (A) : (B))
#define MIN(A, B)         ((A) > (B) ? (B) : (A))
//#define ABS(A, B) ((A) > (B) ? (A - B) : (B - A))

void warn(char *, ...);
__attribute__((noreturn))
void err(char *, ...);
void copy_prop(char *, char *, int , int , int);

#endif
