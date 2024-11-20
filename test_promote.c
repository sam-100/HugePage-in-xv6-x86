#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4*1024
#define HUGEPGSIZE 1024*PGSIZE

// #define BUFFER_SIZE (20*HUGEPGSIZE)
#define INT_SIZE 4

void print(const int *arr, int size, int jump) {
    for(int i=0; i<size; i += jump) 
        printf(1, "%d, ", arr[i]);
    printf(1, "\n");
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf(1, "Usage: %s <size_in_MBs>\n", argv[0]);
        exit();
    }

    int buffer_size = atoi(argv[1])/4*HUGEPGSIZE;
    int *num = (int *)malloc(buffer_size);
    for(int i=0; i<buffer_size/INT_SIZE; i++)
        num[i] = i%10;
    
    printf(1, "Before page promotion: ");
    print(num, buffer_size/INT_SIZE, HUGEPGSIZE/INT_SIZE);

    if(promote(num, buffer_size) != 0)
    {
        printf(1, "promote() syscall failed\n");
        return 0;
    }
    
    printf(1, "After page promotion: ");
    print(num, buffer_size/INT_SIZE, HUGEPGSIZE/INT_SIZE);

    printf(1, "success!\n");

    exit();
}