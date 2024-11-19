#include "types.h"
#include "user.h"


int main(int argc, char **argv) {
    printf(1, "%d free pages left in the memory.\n", get_free_pa_space());
    return 0;
}