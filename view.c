/* --------------------------------------------------------------------------------------------
                                     DEFINITIONS
-------------------------------------------------------------------------------------------- */

#define handle_error(msg) \
                do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define MAX_SHM_PATH_LENGTH 50


/* --------------------------------------------------------------------------------------------
                                     INCLUDES
-------------------------------------------------------------------------------------------- */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <signal.h>
#include "master_view.h"
#include <string.h>


/* --------------------------------------------------------------------------------------------
                                     PROTOTYPES
-------------------------------------------------------------------------------------------- */

void setup(char *shm_path);

void sigint_handler(int sig);

void finish();

/* --------------------------------------------------------------------------------------------
                                     GLOBAL VARIABLES
-------------------------------------------------------------------------------------------- */

// This variables have been made global because they are needed by SIGINT handler, wich
// can't receive custom parameters

char *base; sem_t *read_bytes; sem_t *write_bytes;

/* --------------------------------------------------------------------------------------------
                                     FUNCTIONS
-------------------------------------------------------------------------------------------- */

int main(int argc,char **argv){

    // Get the name of shared memory

    char shm_path[MAX_SHM_PATH_LENGTH]={0};

    if(argc < 2){
        read(STDIN_FILENO, shm_path, MAX_SHM_PATH_LENGTH);
    }else{
        memccpy(shm_path, argv[1], 0, MAX_SHM_PATH_LENGTH);
    }

    
    // Set up shared memory, semaphores and sigint handler
    
    setup(shm_path);

    // Read shared memory buffer

    int pos = 0;
    while(1){
        sem_wait(read_bytes);

        char c = base[pos++ % SHM_SIZE];
        
        if(c == '\n'){
            putchar(c);
            continue;
        }

        else if(c == EOT)
            finish(base, read_bytes, write_bytes);
        
        else if (c==',')
            putchar('\n');
        
        else
            putchar(c);

        sem_post(write_bytes);
    } 


    // Close shared memory and semaphores

    finish();

}


void setup(char *shm_path){

    // Setup shared memory

    int shm_fd= shm_open(shm_path,O_RDONLY,0666);
    if(shm_fd==-1){
        handle_error("shm_open");
    }

    base = (char*) mmap(NULL,SHM_SIZE,PROT_READ,MAP_SHARED,shm_fd,0);
    if(base == MAP_FAILED){
        handle_error("mmap");
    }

    if(close(shm_fd) == -1){
        handle_error("close shm fd");
    }

    // Setup semaphores

    write_bytes = sem_open(SEM_WRITE_BYTES, O_RDWR);
    read_bytes= sem_open(SEM_READ_BYTES, O_RDWR);

    if(write_bytes==SEM_FAILED){
        handle_error("sem_open for write bytes sem");
    }

    if(read_bytes==SEM_FAILED){
        handle_error("sem_open for read bytes sem");
    }

    // Setup sigint handler

    struct sigaction _sigact;
    memset(&_sigact, 0, sizeof(_sigact));

    _sigact.sa_handler = sigint_handler;
    sigaction(SIGINT, &_sigact, NULL);

}


void sigint_handler(int sig){
    finish();
}


void finish(){

    // Close shared memory

    if(munmap(base, SHM_SIZE) == -1){
        handle_error("munmap");
    }

    // Close semaphores

    if(sem_close(read_bytes) == -1){
        handle_error("close read_bytes sem");
    }

    if(sem_close(write_bytes) == -1){
        handle_error("close write_bytes sem");
    }

    exit(EXIT_SUCCESS);

}