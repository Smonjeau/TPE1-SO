#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include "master_view.h"
#include <string.h>






#define handle_error(msg) \
                do { perror(msg); exit(EXIT_FAILURE); } while (0)


void finish(char *shm_base, sem_t *sem_read_bytes, sem_t *sem_write_bytes);


int main(int argc,char **argv){

    char * shm_path = argv[1];
        
    int shm_fd= shm_open(shm_path,O_RDONLY,0666);
    if(shm_fd==-1){
        handle_error("shm_open");
    }

    struct stat sb;
    if (fstat(shm_fd, &sb) == -1){
        handle_error("fstat");
    }

    sem_t * write_bytes = sem_open(SEM_WRITE_BYTES, O_RDWR);
    sem_t * read_bytes= sem_open(SEM_READ_BYTES, O_RDWR);

    if(write_bytes==SEM_FAILED){
        handle_error("sem_open for write bytes sem");
    }

    if(read_bytes==SEM_FAILED){
        handle_error("sem_open for read bytes sem");
    }

    char * base = (char*) mmap(NULL,sb.st_size,PROT_READ,MAP_SHARED,shm_fd,0);
    if(base == MAP_FAILED){
        handle_error("mmap");
    }

    int pos = 0;
    while(1){
        
    

        while(1){
            sem_wait(read_bytes);

            char c = base[pos++ % SHM_SIZE];
            
            if(c == '\n'){
                putchar(c);
                break;
            }

            else if(c == EOT)
                finish(base, read_bytes, write_bytes);
            
            
            else if (c==',') {
                putchar('\n');
            }
            else
                putchar(c);

            sem_post(write_bytes);
        }

       

    }    

    return 0;
}



void finish(char *shm_base, sem_t *sem_read_bytes, sem_t *sem_write_bytes){

    // Close shared memory

    if(munmap(shm_base, SHM_SIZE) == -1){
        handle_error("munmap");
    }

    // Close semaphores

    if(sem_close(sem_read_bytes) == -1){
        handle_error("close read_bytes sem");
    }

    if(sem_close(sem_write_bytes) == -1){
        handle_error("close write_bytes sem");

    }

    exit(EXIT_SUCCESS);

}

/* 
        sem_wait(read_bytes);
        for(i=0 ;base[pos%SHM_SIZE]!='\n'; pos++,i++){
            sem_wait(read_bytes);
            if(i==0)
                putchar('\n');
            if ((c=base[pos%SHM_SIZE]) ==EOT){
                printf("\nme boi\n");
                exit(0);   
            }
            else if (c==',' || (c==0 ) )
                putchar('\n');
            else 
                putchar(c);
            sem_post(write_bytes);
        }
        sem_post(write_bytes);
        pos++; */