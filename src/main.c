#include <stdio.h>
#ifndef APP_VERSION
#error "APP_VERSION is not defined"
#endif

int main(void)
{
    printf("jsoncheck %s - project skeleton\n", APP_VERSION);
    return 0;
}
