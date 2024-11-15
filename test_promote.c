#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4*1024
#define HUGEPGSIZE 1024*PGSIZE

#define BUFFER_SIZE (HUGEPGSIZE+HUGEPGSIZE)
#define INT_SIZE 4
int main(int argc, char *argv[])
{
    int *num = (int *)malloc(BUFFER_SIZE);
    for(int i=0; i<BUFFER_SIZE/INT_SIZE; i++)
        num[i] = i;
    
    printf(1, "Before page promotion: \n");
    for(int i=0; i<BUFFER_SIZE/4; i += HUGEPGSIZE/4)
        printf(1, "%d | ", num[i]);
    printf(1, "\n");

    // if(getpagesize(num))
    //     printf(1, "Huge page = Enabled\n");
    // else 
    //     printf(1, "Huge page = Disabled\n");

    if(promote(num, BUFFER_SIZE) != 0)
    {
        printf(1, "promote() syscall failed\n");
        return 0;
    }
    
    printf(1, "\nAfter page promotion: \n");
    for(int i=0; i<BUFFER_SIZE/4; i += HUGEPGSIZE/4)
        printf(1, "%d\t", num[i]);
    printf(1, "\n");


    // if(getpagesize(num))
    //     printf(1, "Huge page = Enabled\n");
    // else 
    //     printf(1, "Huge page = Disabled\n");

    printf(1, "success!\n");

    return 0;
}