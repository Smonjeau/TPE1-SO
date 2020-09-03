#define SLAVES_QTY 3
#define FILES_TO_DELEGATE 1

#define SLAVE_READ_TIMEOUT_USEC 100
#define MAX_MESSAGE_LEN 100

#define SHM_NAME "/master-view"
#define SHM_SIZE 1024

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

char * setup_shm();
void create_slaves(int sm_fds[][2], int ms_fds[][2], int slave_pids[]);
void handle_slaves(int sm_fds[][2], int ms_fds[][2], char *shm_base, int nfiles, char **files);
void kill_slaves(int slave_pids[]);
void close_shm(char *shm_base);


int main(int argc, char **argv){

    if(argc < 2){
        printf("Usage: ./master f1 ... fn\n");
    }

    int slave_pids[SLAVES_QTY];
    int sm_fds[SLAVES_QTY][2];      // Slave -> Master pipes
    int ms_fds[SLAVES_QTY][2];      // Master -> Slave pipes
    
    char *shm_base = setup_shm();

    create_slaves(sm_fds, ms_fds, slave_pids);
    
    handle_slaves(sm_fds, ms_fds, shm_base, argc-1, argv+1);

    kill_slaves(slave_pids);

    close_shm(shm_base);

}


char * setup_shm(){

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if(shm_fd == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if(ftruncate(shm_fd, SHM_SIZE)==-1){
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    char * shm_base = (char *) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_base == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if(close(shm_fd) == -1){
        perror("close shm_fd");
        exit(EXIT_FAILURE);
    }

    return shm_base;

}


void create_slaves(int sm_fds[][2], int ms_fds[][2], int slave_pids[]){

    for(int i=0; i<SLAVES_QTY; i++){

        pipe(sm_fds[i]);
        pipe(ms_fds[i]);

        int npid = fork();

        if(npid == -1){

            perror("Fork");

        }else if(npid == 0){

            close(sm_fds[i][0]);
            close(ms_fds[i][1]);

            char wr_fd_str[3], rd_fd_str[3], slave_id_str[3];
            sprintf(wr_fd_str, "%d", sm_fds[i][1]);
            sprintf(rd_fd_str, "%d", ms_fds[i][0]);
            sprintf(slave_id_str, "%d", i);

            char *slave_args[] = {"./slave.out", wr_fd_str, rd_fd_str, slave_id_str, NULL};
            execv("./slave.out", slave_args);

            perror("Exec");

        }else{

            slave_pids[i] = npid;

            close(ms_fds[i][0]);
            close(sm_fds[i][1]);

        }

    }

}


void handle_slaves(int sm_fds[][2], int ms_fds[][2], char *shm_base, int nfiles, char **files){

    int filen=0, pending_jobs=0;
    while(filen<nfiles || pending_jobs>0){

        // Populate set of file descriptors to read

        fd_set fdset;
        FD_ZERO(&fdset);
        for(int i=0; i<SLAVES_QTY; i++){
            FD_SET(sm_fds[i][0], &fdset);
        }

        // Set read timeout

        struct timeval tv;
        tv.tv_usec = SLAVE_READ_TIMEOUT_USEC;
        
        // Check available file descriptors

        int available_fds = select(sm_fds[SLAVES_QTY-1][0]+1, &fdset, NULL, NULL, &tv);

        switch(available_fds){

            case -1:
                perror("Select");
                break;

            default:

                // Iterate pipes with available read data

                for(int i=0; i<SLAVES_QTY; i++){
                    if(FD_ISSET(sm_fds[i][0], &fdset)){

                        char input[MAX_MESSAGE_LEN];
                        read(sm_fds[i][0], input, MAX_MESSAGE_LEN);

                        if(strcmp(input, "REQ")==0){

                            // Slave requested a job

                            if(filen >= nfiles){
                                write(ms_fds[i][1], "EMPTY", MAX_MESSAGE_LEN);
                                continue;
                            }

                            // Create output

                            char output[MAX_MESSAGE_LEN];
                            int outpos = sprintf(output, "%s", files[filen++]);

                            int last_filen = filen+FILES_TO_DELEGATE-1;
                            while(filen<last_filen && filen<nfiles){
                                outpos += sprintf(output+outpos, ",%s", files[filen++]);
                            }

                            write(ms_fds[i][1], output, MAX_MESSAGE_LEN);
                            pending_jobs += 1;

                        }else{

                            // Slave sent a job response

                            printf("@M - S%d: %s\n", i, input);

                            sprintf(shm_base, "%s", input);

                            write(ms_fds[i][1], "ACK", 4);
                            pending_jobs -= 1;

                        }

                    }
                }
        }

    }

}


void kill_slaves(int slave_pids[]){

    for(int i=0; i<SLAVES_QTY; i++){
        kill(slave_pids[i], SIGKILL);
    }

}


void close_shm(char *shm_base){


    if(munmap(shm_base, SHM_SIZE) == -1){
        perror("munmap");
        exit(1);
    }

}