#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "helpers.h"

void warn(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

__attribute__((noreturn))
void err(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void copy_prop(char *dest, char *src, int len, int idx, int num_itm)
{
    if (num_itm <= 1) {
        strncpy(dest, src, MIN(len, MAXLEN));
        dest[len] = '\0';
    } else {
        int pos = 0, cnt = 0;
        while (cnt < idx && cnt < (num_itm - 1) && pos < len) {
            pos += strlen(src + pos) + 1;
            cnt++;
        }
        if (cnt == (num_itm - 1))
            copy_prop(dest, src + pos, len - pos, 0, 1);
        else
            strncpy(dest, src + pos, MAXLEN);
    }
}
