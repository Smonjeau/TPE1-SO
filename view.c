#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_SIZE 1024
#define SEM1_NAME "/write_bytes"
#define SEM2_NAME "/read_bytes"


void proccess_byte(char c, char * buffer);

void handle_error(char * msg);

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

    sem_t * write_bytes = sem_open(SEM1_NAME, O_RDWR);
    sem_t * read_bytes= sem_open(SEM2_NAME, O_RDWR);

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
        
        char file_data[512]={0}; int file_pos=0;

        while(1){
            sem_wait(read_bytes);

            char c = base[pos++ % SHM_SIZE];
            
            if(c == '\n')
                break;

            if(c == 4)
                finish(base, read_bytes, write_bytes);
            
            file_data[file_pos++] = c;

            sem_post(write_bytes);
        }

        printf("%s\n", file_data);

    }    

    return 0;
}


void handle_error(char * msg){
    perror(msg);
    exit(EXIT_FAILURE);
}


void finish(char *shm_base, sem_t *sem_read_bytes, sem_t *sem_write_bytes){

    // Close shared memory

    if(munmap(shm_base, SHM_SIZE) == -1){
        perror("munmap");
        exit(1);
    }

    // Close semaphores

    if(sem_close(sem_read_bytes) == -1){
        perror("closing read_bytes semaphore");
        exit(EXIT_FAILURE);
    }

    if(sem_close(sem_write_bytes) == -1){
        perror("closing write_bytes semaphore");
        exit(EXIT_FAILURE);
    }

    exit(0);

}