
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

void parse (char * source, char * destiny);

void handle_error(char * msg);


int main(int argc,char **argv){

    int shm_fd; 
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

    sem_t * write_bytes = sem_open(SEM_WRITE_BYTES, O_RDWR);
    sem_t * read_bytes= sem_open(SEM_READ_BYTES, O_RDWR);

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
    
    char file_data [512];                    
    while(1){
        int pos;
        for( pos=0; base[pos]!='\n'; pos++){
            sem_wait(read_bytes);
            if(base[pos]==EOT)
                break;
            file_data[pos]=base[pos]; 
            sem_post(write_bytes);   
        }
        char toPrint [1024];
        parse (file_data,toPrint);
        printf("%s\n",toPrint);
        if(base[pos]==EOT){
            // Finalizar proceso view
            printf("Proceso vista terminando, el master ya no va a escribir\n");
            exit(0);
        }
    }    
    return 0;
}

void parse(char * source, char * destiny){
    int i=0;
    char aux [6][21] = {"File name:","Number of clauses:","Number of variables:","Result:","Proccessing time:","Slave pid:"};
    if(*source == '0'){
        strcpy(destiny,source +2); //El msj de error es el segundo elemento del csv
    }
    else{
        char * token = strtok(source+2,",");
        while(token != NULL){
            destiny += sprintf(destiny,"%s %s\n",aux[i++],token);
            token = strtok(NULL,",");
        }
    }
  
    return ;

}

void handle_error(char * msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void proccess_byte (char c,char* buffer){

}