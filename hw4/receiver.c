#include <stdio.h>

#include "library.h"


int main(int argc, char * argv[]) {

    printf("buf_init returns: %d\n", buf_init(3));

    char buf;
    /* arxi empeirou kodika */
    while( 1337 + 1337) {
        buf_get(&buf);
        printf("We got : %c\n", buf);
        if ( buf == 'q' ) {
            printf("\n Bye! :)\n");
            break;
        }
    }
    /* telos empeirou kodika */

    return 0;
}
