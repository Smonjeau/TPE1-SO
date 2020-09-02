#define SLAVES_QTY 5
#define MAX_PATH_LEN 20
#define MAX_RESPONSE_LEN 100


#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

void create_slaves(int sm_fds[][2], int ms_fds[][2]);
void read_slaves(int sm_fds[][2], int ms_fds[][2]);


int main(){

    int sm_fds[SLAVES_QTY][2];
    int ms_fds[SLAVES_QTY][2];

    create_slaves(sm_fds, ms_fds);
    
    read_slaves(sm_fds, ms_fds);

}


void create_slaves(int sm_fds[][2], int ms_fds[][2]){

    for(int i=0; i<SLAVES_QTY; i++){

        pipe(sm_fds[i]);
        pipe(ms_fds[i]);

        if(fork() == 0){

            close(sm_fds[i][0]);
            close(ms_fds[i][1]);

            char wr_pid_str[3], rd_pid_str[3], path[MAX_PATH_LEN];
            sprintf(wr_pid_str, "%d", sm_fds[i][1]);
            sprintf(rd_pid_str, "%d", ms_fds[i][0]);
            sprintf(path, "FILE_%d", i);

            char *slave_args[] = {"./slave.out", wr_pid_str, rd_pid_str, path, NULL};
            execv("./slave.out", slave_args);

        }else{

            close(ms_fds[i][0]);
            close(sm_fds[i][1]);

        }

    }

}


void read_slaves(int sm_fds[][2], int ms_fds[][2]){

    while(1){

        fd_set fdset;
        FD_ZERO(&fdset);
        for(int i=0; i<SLAVES_QTY; i++){
            FD_SET(sm_fds[i][0], &fdset);
        }

        struct timeval tv;
        tv.tv_usec = 500;

        int available_fds = select(sm_fds[4][0]+1, &fdset, NULL, NULL, &tv);
        switch(available_fds){

            case -1:
                perror("Error reading slaves");
                break;

            case 0:
                //printf("No data available");
                break;

            default:

                for(int i=0; i<SLAVES_QTY; i++){
                    if(FD_ISSET(sm_fds[i][0], &fdset)){

                        char response[MAX_RESPONSE_LEN];
                        int len = read(sm_fds[i][0], response, MAX_RESPONSE_LEN);

                        printf("@M - S%d: %s\n", i, response);

                    }
                }
        }

    }

}