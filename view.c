#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdlib.h>
#include <unistd.h>

int main(int argc,char **argv){
    char * shm_path=malloc(sizeof(char)*20);
    int shm_fd,aux;
    char * base;
    struct stat sb;
    if(argc==2)
        shm_path=argv[1];
    else
        scanf("%s",shm_path);
    shm_fd= shm_open(shm_path,O_RDONLY,0666); //Investigar bien este 0666
    if(shm_fd==-1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    //queremos saber el tamaño del archivo
    if (fstat(shm_fd, &sb) == -1){
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    //vamos a leer hasta EOF
    //Al pasar NULL como primer parámetro, el kernel se encarga de buscar lugar donde mapear
    base = (char*) mmap(NULL,sb.st_size,PROT_READ,MAP_SHARED,shm_fd,0);
    if(base == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    aux = write(STDOUT_FILENO,base,sb.st_size);
    if (aux != sb.st_size) {
        if (aux == -1){
            perror("write");
            exit(EXIT_FAILURE);         
        }
        fprintf(stderr, "No se pudo escribir todo\n");
        exit(EXIT_FAILURE);
    }
    free(shm_path);
    return 0;
}