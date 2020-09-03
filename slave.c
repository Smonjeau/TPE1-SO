#define MAX_MESSAGE_LEN 100

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>


void process(char *input, char *output, int slave_id);


int main(int argc, char ** argv){

    int wr_fd = atoi(argv[1]);
    int rd_fd = atoi(argv[2]);
    int slave_id = atoi(argv[3]);

    while(1){

        // Request a file
        
        write(wr_fd, "REQ", MAX_MESSAGE_LEN);

        char input[MAX_MESSAGE_LEN];
        read(rd_fd, input, MAX_MESSAGE_LEN);

        if(strcmp(input, "EMPTY")!=0){
            // Process the files

            char output[MAX_MESSAGE_LEN];
            process(input, output, slave_id);

            // Return the result
            
            write(wr_fd, output, MAX_MESSAGE_LEN);
        }

        while(strcmp(input, "ACK") != 0){
            read(rd_fd, input, MAX_MESSAGE_LEN);
        }

    }
    
}


void process(char *input, char *output, int slave_id){

    printf("@S%d: %s\n", slave_id, input);
    sprintf(output, "**%s**", input);

    sleep(3);

    // Split the input and process each path, then return all answers in CSV format

}