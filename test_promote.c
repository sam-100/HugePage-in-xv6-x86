#include "types.h"
#include "stat.h"
#include "user.h"

#define M 1024*1024

int main(int argc, char *argv[])
{
    int *num = (int *) malloc(4*M);
    for(int i=0; i<M; i++)
        num[i] = i;
    
    if(getpagesize(num))
        printf(1, "Huge page = Enabled\n");
    else 
        printf(1, "Huge page = Disabled\n");

    // promote(num);

    // printf("page size after promote(): %d\n", getpagesize(num));

    return 0;
}