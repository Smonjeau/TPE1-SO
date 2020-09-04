#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define SEM1_NAME "/write_bytes"

#define SEM2_NAME "/read_bytes"

void proccess_byte(char c, char * buffer);

void handle_error(char * msg);

int main(int argc,char **argv){

    int shm_fd; //aux;
    char * base;
    struct stat sb;

    char * shm_path = argv[1];
        
    shm_fd= shm_open(shm_path,O_RDONLY,0666); //Investigar bien este 0666
    if(shm_fd==-1){
        handle_error("shm_open");
    }

    //queremos saber el tamaÃ±o del archivo
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

    //vamos a leer hasta EOF
    //Al pasar NULL como primer parÃ¡metro, el kernel se encarga de buscar lugar donde mapear
    base = (char*) mmap(NULL,sb.st_size,PROT_READ,MAP_SHARED,shm_fd,0);
    if(base == MAP_FAILED){
        handle_error("mmap");
    }

/*     aux = write(STDOUT_FILENO,base,sb.st_size);
    if (aux != sb.st_size) {
        if (aux == -1){
            handle_error("write");   
        }
        fprintf(stderr, "Could not write everything\n");
        exit(EXIT_FAILURE);
    } */
    // char local_buffer [512];
    int index = 0;
    while(1){
        sem_wait(read_bytes);
        printf("%c",base[index++]); 
        sem_post(write_bytes);   
    }    
    return 0;
}

void handle_error(char * msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void proccess_byte (char c,char* buffer){

}