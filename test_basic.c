// includes
#include "types.h"
#include "user.h"


// definitions 

void error(const char *message) {
    printf(1, "%s\n", message);
    exit();
}

int test_integrity(const int *arr, int size) {
    for(int i=0; i<size; i++)
        if(arr[i] != i%256)
            return 1;
    return 0;
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

    // 3. invoke promote system call 
    if(promote(arr, size_in_bytes))
        error("Error: promote syscall failed.");
    printf(1, "promote() system call success.\n");

    // 4. check integrity
    if(test_integrity(arr, size_in_bytes/sizeof(int)))
        error("Error: integrity failure.\n");
    printf(1, "Integrity test successful after promote.\n");
    
    printf(1, "%d Huge pages after promote()\n", huge_page_count(arr, size_in_bytes));

    // 5. invoke demote system call 
    if(demote(arr, size_in_bytes))
        error("Error: demote syscall failed.");
    printf(1, "demote() system call success.\n");

    // 6. check integrity
    if(test_integrity(arr, size_in_bytes/sizeof(int)))
        error("Error: integrity failure.\n");
    printf(1, "Integrity test successful after demote.\n");

    printf(1, "%d Huge pages after demote().\n", huge_page_count(arr, size_in_bytes));

    exit();
}