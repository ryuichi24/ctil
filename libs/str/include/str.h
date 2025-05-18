#ifndef STR_H
#define STR_H

#include <string.h>

// https://www.bytesbeneath.com/p/custom-strings-in-c?utm_source=chatgpt.com
typedef struct
{
    char *data;
    size_t length;
} String;

#define String(x) \
    (String){ x, strlen(x) }

#endif /* STR_H */