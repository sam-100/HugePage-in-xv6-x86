#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4*1024
#define HUGEPGSIZE 1024*PGSIZE

#define BUFFER_SIZE (5*HUGEPGSIZE)
#define INT_SIZE 4

void print(const int *arr, int size, int jump) {
    for(int i=0; i<size; i += jump) 
        printf(1, "%d, ", arr[i]);
    printf(1, "\n");
}

int main(int argc, char *argv[])
{
    int *num = (int *)malloc(BUFFER_SIZE);
    for(int i=0; i<BUFFER_SIZE/INT_SIZE; i++)
        num[i] = i%10;
    
    printf(1, "Before page promotion: ");
    print(num, BUFFER_SIZE/INT_SIZE, HUGEPGSIZE/INT_SIZE);

    if(promote(num, BUFFER_SIZE) != 0)
    {
        printf(1, "promote() syscall failed\n");
        return 0;
    }
    
    printf(1, "After page promotion: ");
    print(num, BUFFER_SIZE/INT_SIZE, HUGEPGSIZE/INT_SIZE);

    printf(1, "success!\n");

    return 0;
}