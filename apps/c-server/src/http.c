#include "http.h"

int is_valid_port(int port)
{
    if (port <= 0 || port > 65535)
    {
        return 1;
    }

    return 0;
}