// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/* --------------------------------------------------------------------------------------------
                                     DEFINITIONS
-------------------------------------------------------------------------------------------- */

#define SLAVES_QTY 3                          // Number of slaves to create (fixed amount)
#define FILES_TO_DELEGATE 1                   // Number of files given to each slave per request

#define SLAVE_READ_TIMEOUT_USEC 100           // Max time to wait for a slave to write on the pipe
#define MAX_MESSAGE_LEN 1000                   // Max extension of messages between master/slaves

#define SHM_NAME "/master-view"               // Master-view shared memory name
#define NAME_SIZE 12
#define DELAY_FOR_VIEW 6

#define LOGFILE_NAME "resultados.txt"

#define handle_error(msg) \
                do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* --------------------------------------------------------------------------------------------
                                     INCLUDES
-------------------------------------------------------------------------------------------- */

#include "master_view.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

/* --------------------------------------------------------------------------------------------
                                     PROTOTYPES
-------------------------------------------------------------------------------------------- */

void setup();

void create_slaves(int sm_fds[][2], int ms_fds[][2]);

void handle_slaves(int sm_fds[][2], int ms_fds[][2], int nfiles, char **files);

void kill_slaves();

void write_buffer(char c);

void finish();

void sigint_handler(int sig);

/* --------------------------------------------------------------------------------------------
                                     GLOBAL VARIABLES
-------------------------------------------------------------------------------------------- */

// This variables have been made global because they are needed by SIGINT handler, wich
// can't receive custom parameters

char *shm_base=NULL; sem_t *sem_read_bytes=NULL, *sem_write_bytes=NULL;
int slave_pids[SLAVES_QTY]; FILE *logfile_fd;

/* --------------------------------------------------------------------------------------------
                                     FUNCTIONS
-------------------------------------------------------------------------------------------- */

int main(int argc, char **argv){

    if(argc < 2){
        printf("Usage: ./master f1 ... fn\n");
    }

    int sm_fds[SLAVES_QTY][2];      // Slave -> Master pipes
    int ms_fds[SLAVES_QTY][2];      // Master -> Slave pipes
   
    setup();

    create_slaves(sm_fds, ms_fds);
    
    handle_slaves(sm_fds, ms_fds, argc-1, argv+1);

    finish();

    return 0;

}


void setup(){
    
    // Create log file

    logfile_fd = fopen(LOGFILE_NAME, "w+");
    if(logfile_fd == NULL){
        handle_error("Opening log file");
    }

    // Setup shared memory

     int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if(shm_fd == -1){
        handle_error("Opening shared memory");
    }

    if(ftruncate(shm_fd, SHM_SIZE)==-1){
        handle_error("Truncating shared memory");
    }

    shm_base = (char *) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_base == MAP_FAILED){
        handle_error("Mapping shared memory");
    }

    if(close(shm_fd) == -1){
        handle_error("Closing shared memory file descriptor");
    }

    // Setup semaphores

    sem_read_bytes = sem_open(SEM_READ_BYTES, O_CREAT | O_RDWR, 0666, 0);
    if(sem_read_bytes == SEM_FAILED){
        handle_error("Opening read_bytes semaphore");
    }

    sem_write_bytes = sem_open(SEM_WRITE_BYTES, O_CREAT | O_RDWR, 0666, SHM_SIZE);
    if(sem_write_bytes == SEM_FAILED){
        handle_error("Opening write_bytes semaphore");
    }

    // Wait some time for the view to connect and print shm name
    
    write(STDOUT_FILENO, SHM_NAME,NAME_SIZE);
    sleep(DELAY_FOR_VIEW);

    // Set up SIGINT handler

    struct sigaction _sigact;
    memset(&_sigact, 0, sizeof(_sigact));

    _sigact.sa_handler = sigint_handler;
    sigaction(SIGINT, &_sigact, NULL);

}


void sigint_handler(int sig){
    
    finish();

}


void create_slaves(int sm_fds[][2], int ms_fds[][2]){

    for(int i=0; i<SLAVES_QTY; i++){

        pipe(sm_fds[i]);
        pipe(ms_fds[i]);

        int npid = fork();

        if(npid == -1){

            handle_error("Forking master process");

        }else if(npid == 0){

            close(sm_fds[i][0]);
            close(ms_fds[i][1]);

            char wr_fd_str[3], rd_fd_str[3], slave_id_str[3];
            sprintf(wr_fd_str, "%d", sm_fds[i][1]);
            sprintf(rd_fd_str, "%d", ms_fds[i][0]);
            sprintf(slave_id_str, "%d", i);

            char *slave_args[] = {"./slave.out", wr_fd_str, rd_fd_str, slave_id_str, NULL};
            execv("./slave.out", slave_args);

            handle_error("Executing a slave");

        }else{

            slave_pids[i] = npid;

            close(ms_fds[i][0]);
            close(sm_fds[i][1]);

        }

    }

}


void handle_slaves(int sm_fds[][2], int ms_fds[][2], int nfiles, char **files){

    // The master must listen to slaves until there are no more files nor pending jobs

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
        memset(&tv, 0, sizeof(struct timeval));

        tv.tv_usec = SLAVE_READ_TIMEOUT_USEC;
        
        // Check available file descriptors

        int available_fds = select(sm_fds[SLAVES_QTY-1][0]+1, &fdset, NULL, NULL, &tv);

        switch(available_fds){

            case -1:
                handle_error("Selecting available file descriptors");
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
                                write(ms_fds[i][1], "EMPTY", 6);
                                continue;
                            }

                            // Create files message for slave

                            char output[MAX_MESSAGE_LEN] = {0};
                            int outpos = sprintf(output, "%s", files[filen++]);

                            int last_filen = filen+FILES_TO_DELEGATE-1;
                            while(filen<last_filen && filen<nfiles){
                                outpos += sprintf(output+outpos, ",%s", files[filen++]);
                            }

                            // printf("For S%d: %s\n", i, output);

                            write(ms_fds[i][1], output, MAX_MESSAGE_LEN);
                            pending_jobs += 1;

                        }else{

                            // Slave sent a job response

                            // printf("S%d: %s\n", i, input);

                            for(int input_pos=0; input[input_pos] != 0; input_pos++){
                                write_buffer(input[input_pos]);
                                fputc(input[input_pos], logfile_fd);
                            }

                            write_buffer('\n');
                            fputc('\n', logfile_fd);

                            write(ms_fds[i][1], "ACK", 4);
                            pending_jobs -= 1;

                        }

                    }
                }
        }

    }

}


void write_buffer(char c){

    static int buff_pos = 0;
    sem_wait(sem_write_bytes);

    sprintf(shm_base + (buff_pos++) % SHM_SIZE, "%c", c);

    sem_post(sem_read_bytes);

}


void kill_slaves(){

    for(int i=0; i<SLAVES_QTY; i++){
        kill(slave_pids[i], SIGKILL);
    }

}


void finish(){

    
    kill_slaves();

    // Close log file

    if(fclose(logfile_fd) == -1){
        handle_error("Closing log file");
    }

    // Signal view end of transmission

    write_buffer(EOT);

    // Close shared memory

    if(munmap(shm_base, SHM_SIZE) == -1){
        handle_error("Unmapping shared memory");
    }
    if(shm_unlink(SHM_NAME)==-1)
        handle_error("shm_unlink");

    // Close semaphores

    if(sem_close(sem_read_bytes) == -1){
        handle_error("Closing read_bytes semaphore");
    }

    if(sem_close(sem_write_bytes) == -1){
        handle_error("Closing write_bytes semaphore");
    }
    if(sem_unlink(SEM_READ_BYTES)==-1)
        handle_error("unlink for read bytes sem");
    if(sem_unlink(SEM_WRITE_BYTES)==-1)
        handle_error("unlink for write bytes sem");    
    
    exit(EXIT_SUCCESS);

}