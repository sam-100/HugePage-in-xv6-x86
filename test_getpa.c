#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
    int *num = (int *) malloc(4);
    *num = 100;

    printf(1, "num = %d: va = %p, pa = %p\n", *num, num, getpa(num));
    exit();
    return 0;
}