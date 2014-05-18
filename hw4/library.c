#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "debug.h"
#include "library.h"

#define RET_FAIL -1
#define RET_SUCCESS 1
#define RET_PASS 0

#define is_mem_ok(M) (M != NULL && M!=(void*)-1)

/*#define NDEBUG*/
/* ipcs -m !!!!! Terminal command to show all shared memory segments!!!
 * ipcrm -m <shmid> !!!! Terminal command to destroy segment!! */

int shmid, n;
shm_st *shm_segment = NULL;

int buf_init(int n) {
    debug("Lean struct size: %zu", sizeof(*shm_segment) );
    /* we need n+1 size since we have to sacriffice 1 spot in the
     * array so we can use circular buffer related, modulo calculations
     * properly */
    size_t totalSize = sizeof(shm_st) + (n+1)*sizeof(char);
    debug("Total size: %zu", totalSize);

    int retValue = RET_SUCCESS;

    int _shmid = shmget(SHM_KEY, totalSize, IPC_CREAT | IPC_EXCL | S_IRWXU);
    if( _shmid < 0 ) {
        log_warn("_shmid < 0!\n");
        if( errno == EEXIST ) {
            _shmid = shmget(SHM_KEY, totalSize, 0);
            retValue = RET_PASS;
        }
        else {
            log_err("shmget");
            return RET_FAIL;
        }
    }
    debug("_shmid: %d\n", _shmid);

    shmid = _shmid;
    shm_segment = (shm_st*)shmat(shmid, NULL, 0);
    if( shm_segment == (void*)-1 ) {
        log_err("shmat");
        return RET_FAIL;
    }

    /* Init struct */
    shm_segment->size = n+1;
    shm_segment->in = shm_segment->out = 0;

    debug("Attached ID: [%d] at [%p].\n", shmid, shm_segment);
    return retValue;
}

int buf_destroy(void) {
    int reValue = shmctl(shmid, IPC_RMID, NULL);
    if ( reValue < 0 ) {
        if ( errno == EIDRM ) {
            log_warn("shmid: %d already removed!", shmid);
            return RET_PASS;
        }

        log_err("shmctl");
        return RET_FAIL;
    }
    debug("segment: [%d] scheduled for deletion.", shmid);
    /* Everything went OK */
    return RET_SUCCESS;
}

int abs(int a){
    return a>0? a: -a;
}

int buf_put(char c) {
    if ( !is_mem_ok(shm_segment) ){
        log_err("Buffer not initialized.");
        return RET_FAIL;
    }

    int peek; /* used to safeguard against multiliple calls from N programs */
    /* Increment in */
    peek = (shm_segment->in+1) % shm_segment->size;

    debug("Trying to put char [%c] in %d, in: %d, out: %d", c,
            peek,
            shm_segment->in,
            shm_segment->out
        );

    while ( shm_segment->out - peek == 0){
        /* Busy loop, waiting for buffer to get an empty slot */
    }

    (shm_segment->buf)[(shm_segment->in)] = c;

    debug("We just put char [%c] in buffer @ pos: %d", c, peek);
    shm_segment->in = peek;
    debug("buf_put: positions state--> in: %d, out: %d",
            shm_segment->in,
            shm_segment->out
        );

    return RET_SUCCESS;
}

int buf_get(char *c){
    if ( !is_mem_ok(shm_segment) ){
            log_err("Buffer not initialized.");
            return RET_FAIL;
    }
    debug("Trying to get char from buffer, in: %d, out: %d",
            shm_segment->in,
            shm_segment->out
        );

    while( shm_segment->in - shm_segment->out == 0){
        /* Busy loop, waiting for buffer to fill so we can read */
    }

    /* Extract next character from buffer */
    *c = (shm_segment->buf)[shm_segment->out];

    /* Increment out */
    shm_segment->out = (shm_segment->out + 1) % shm_segment->size;

    debug("Got char %c from buffer, in: %d, out: %d", *c,
        shm_segment->in,
        shm_segment->out
    );


    return RET_SUCCESS;
}



