#define MAX_MESSAGE_LEN 100

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>


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
    char * inputFromGrep = malloc(200 * sizeof(char));
    char outputForMaster[200];
    outputForMaster[0] = 0;
    int forkMinisat, forkGrep;    
	char * token = malloc(100 * sizeof(char));//char token[100];
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

		} else if(forkMinisat == -1) {
			perror("fork");
		}

		//Pipe para comunicar grep y slave
		if(pipe(pipe_fd_grep_slave) == -1) {
			perror("pipe");
			exit(EXIT_FAILURE);
		}

		forkGrep = fork();
		if(forkGrep == 0) { //Proceso que ejcuta grep para filtrar salida del minisat		
			close(pipe_fd_sat_grep[1]); //Cierro lo que no uso
			close(pipe_fd_grep_slave[0]); //Cierro lo que no uso
			if (dup2(pipe_fd_sat_grep[0], 0) < 0 || dup2(pipe_fd_grep_slave[1], 1) < 0) {  
	            perror("dup");
	            exit(EXIT_FAILURE);
	        }
			char *args[] = {"grep", "-o", "-e", "Number of .*[0-9]+", "-e", "CPU time.*", "-e", ".*SATISFIABLE", NULL};
	        if (execv("minisat", args) < 0) {
	        	perror("exec");
	            exit(EXIT_FAILURE);
	        }

		} else if(forkGrep == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}

		//Leo linea por linea lo que imprimio el grep al pipe
		size_t inputFromGrepSize = 200;
		FILE * fp = fdopen(pipe_fd_grep_slave[0], "r");
		while(getline(&inputFromGrep, &inputFromGrepSize, fp) > 0) {
			strcat(outputForMaster, inputFromGrep);
			strcat(outputForMaster, ",");
		}
		outputForMaster[strlen(outputForMaster)-1] = '\n';

		if(fclose(fp) != 0) {
			perror("fclose");
			exit(EXIT_FAILURE);
		}
        



		//Parent, cierro pipes
		close(pipe_fd_sat_grep[0]);
		close(pipe_fd_sat_grep[1]);
		close(pipe_fd_grep_slave[0]);
		close(pipe_fd_grep_slave[1]);

		while ((wait(NULL)) > 0); //Espero a que terminen los 2

		printf("%s", outputForMaster);


		token = strtok(NULL, ",");
	}

}