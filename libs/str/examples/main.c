#include <stdio.h>
#include "str.h"

int main()
{
    String name = String("Hello World!");
    printf("%s\n", name.data);
    return 0;
}