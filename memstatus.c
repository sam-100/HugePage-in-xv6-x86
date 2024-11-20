#include "types.h"
#include "user.h"


int main(int argc, char **argv) {
    printf(1, "Available Physical memory = %d pages\n", get_free_pa_space());
    exit();
}