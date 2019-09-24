#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv) {


    if (argc > 3) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(1);
    }
    return 0;
}