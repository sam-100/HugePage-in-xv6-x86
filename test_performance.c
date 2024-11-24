// includes
#include "types.h"
#include "user.h"

// definitions 
void error(const char *message) {
    printf(1, "%s\n", message);
    exit();
}

// returns time takent to access entire array in miliseconds
int access_time(int *arr, int size) {
    int start, end;
    start = uptime();
    for(int i=0; i<size; i++)
        if(arr[i] != i%256)
            return -1;
    end = uptime();
    return (end-start)*10;
}

int main(int argc, char **argv) {

    if(argc < 2)
    {
        printf(1, "Usage: %s <MegaBytes_to_allocate>\n", argv[0]);
        exit();
    }

    // 1. Declare an array of given size 
    int size_in_bytes = atoi(argv[1])*(1 << 20);
    int *arr = malloc(size_in_bytes);
    
    // 2. Initialzie the array 
    for(int i=0; i < size_in_bytes/sizeof(int); i++)
        arr[i] = i%256;
    printf(1, "Array of size %s MBs initialized.\n", argv[1]);

    int time_before = access_time(arr, size_in_bytes/sizeof(int));
    printf(1, "Access time  = %d ms.\n", time_before);

    // 3. invoke promote system call 
    if(promote(arr, size_in_bytes))
        error("Error: promote syscall failed.");
    printf(1, "promote() system call success.\n");

    // loading TLB
    for(int i=0; i<size_in_bytes/sizeof(int); i++)
        if(arr[i] != i%256)
            return 1;

    int time_after = access_time(arr, size_in_bytes/sizeof(int));
    printf(1, "Access time  = %d ms.\n", time_after);

    // 4. Difference in access time
    if(time_after < time_before)
    {
        int performance = (float)(time_before-time_after)/time_before*100;
        printf(1, "Performance improvement by %d percent.\n", performance);
    }

    // 5. invoke demote system call 
    if(demote(arr, size_in_bytes))
        error("Error: demote syscall failed.");
    exit();
}


