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


void process(char *input, char *output, int slave_id) {
	// Split the input and process each path, then return all answers in CSV format

    //rintf("@S%d: %s\n", slave_id, input);
    //sprintf(output, "**%s**", input);
    int pipe_fd_sat_grep[2];
    int pipe_fd_grep_slave[2];

    int forkMinisat, forkGrep;    
	char token[100];
	token = strtok(input, ",");
	while(token != NULL) {
		//Asumo que no hace falta hacer trim, aunque cualquier cosa es sencillo		

		//Pipe para comunicar minisat y grep
		if(pipe(pipe_fd_sat_grep) == -1) {
			perror("pipe");
			exit(EXIT_FAILURE);
		}
		forkMinisat = fork();
		if(forkMinisat == 0) {
			//Minisat escribe
			close(pipe_fd_sat_grep[0]); //Cierro lo que no uso
			if (dup2(pipe_fd_sat_grep[1], 1) < 0) {  
	            perror("dup");
	            exit(EXIT_FAILURE);
	        }
			char *args[] = {"minisat", token, NULL};
	        if (execv("minisat", args) < 0) {
	        	perror("exec");
	            exit(EXIT_FAILURE);
	        }

		} else if(forkResult == -1) {
			perror("fork");
		}

		forkGrep = fork();
		if(forkGrep == 0) {
			//grep lee y filtra
			close(pipe_fd_sat_grep[1]); //Cierro lo que no uso
			if (dup2(pipe_fd_sat_grep[0], 0) < 0) {  
	            perror("dup");
	            exit(EXIT_FAILURE);
	        }
			char *args[] = {"grep", "-o", "-e", "Number of .*[0-9]+", "-e", "CPU time.*", "-e", ".*SATISFIABLE", NULL};
	        if (execv("minisat", args) < 0) {
	        	perror("exec");
	            exit(EXIT_FAILURE);
	        }

		} else if(forkResult == -1) {
			perror("fork");
		}

		//Parent, cierro pipe
		close(pipe_fd_sat_grep[0]);
		close(pipe_fd_sat_grep[1]);




		token = strtok(NULL, ",");
	}

}