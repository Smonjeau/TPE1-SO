#define MAX_RESPONSE_LEN 100

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>


int main(int argc, char ** argv){

    int wr_fd = atoi(argv[1]);
    int rd_fd = atoi(argv[2]);
    char *path = argv[3];

    int i=0;
    while(1){
        char response[MAX_RESPONSE_LEN];
        sprintf(response, "%s+%d", path, i++);

        write(wr_fd, response, MAX_RESPONSE_LEN);
        sleep(1);
    }
    
}