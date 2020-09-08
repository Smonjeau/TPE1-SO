// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/* --------------------------------------------------------------------------------------------
                                     DEFINITIONS
-------------------------------------------------------------------------------------------- */

#define MAX_SIZE_FOR_MINISAT_OUTPUT 2048
#define MAX_SIZE_FOR_GREP_OUTPUT 1024

#define _GNU_SOURCE

#define handle_error(msg) \
                do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* --------------------------------------------------------------------------------------------
                                     INCLUDES
-------------------------------------------------------------------------------------------- */

#include "master_slave.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

/* --------------------------------------------------------------------------------------------
                                     PROTOTYPES
-------------------------------------------------------------------------------------------- */

void process(char *input, char *output);

/* --------------------------------------------------------------------------------------------
                                     FUNCTIONS
-------------------------------------------------------------------------------------------- */

int main(int argc, char ** argv){

    int wr_fd = atoi(argv[1]);
    int rd_fd = atoi(argv[2]);

    while(1){

        // Request files
        
        write(wr_fd, "REQ", 4);

        char input[MAX_MESSAGE_LEN];
        read(rd_fd, input, MAX_MESSAGE_LEN);

        if(strcmp(input, "EMPTY")!=0){
            // Process the files
        	
            char output[MAX_MESSAGE_LEN]={0};
            process(input, output);

            // Return the result            
            write(wr_fd, output, MAX_MESSAGE_LEN);
        }

		// Wait before requesting new files

        while(strcmp(input, "ACK") != 0){
            read(rd_fd, input, MAX_MESSAGE_LEN);
        }

    }

    exit(0); //Nunca va a llegar
    
}


void process(char *input, char *output) {
	
	// Split the input and process each path, then return all answers in CSV format

    int pipe_fd_sat_grep[2];
    int pipe_fd_grep_slave[2];

    pid_t forkMinisat, forkGrep;  

	char * token = strtok(input, ",");

	while(token != NULL) {

		//Pipe para comunicar minisat y grep
		if(pipe(pipe_fd_sat_grep) == -1) {
			handle_error("Opening minisat pipe");
		}		


		forkMinisat = fork();
		if(forkMinisat == 0) {
			//Minisat escribe
			close(pipe_fd_sat_grep[0]); //Cierro lo que no uso

			if (dup2(pipe_fd_sat_grep[1], 1) < 0) {
	            handle_error("Minisat pipe dup");
	        }	


			char *args[] = {"minisat", token, NULL};
			// printf("proximo: %s\n", token);
			
	        if (execvp("minisat", args) < 0) {
	        	handle_error("Minisat exec");
	        }


			close(pipe_fd_sat_grep[1]);
	        exit(0);

		} else if(forkMinisat == -1) {
			handle_error("Forking for minisat process");
		}

		close(pipe_fd_sat_grep[1]); //La escritura acá no la necesito
		

		//Pipe para comunicar grep y slave
		if(pipe(pipe_fd_grep_slave) == -1) {	
			handle_error("Opening grep pipe");
		}		


		forkGrep = fork();
		if(forkGrep == 0) { //Proceso que ejcuta grep para filtrar salida del minisat		
			close(pipe_fd_sat_grep[1]); //Cierro lo que no uso
			close(pipe_fd_grep_slave[0]); //Cierro lo que no uso

			if (dup2(pipe_fd_grep_slave[1], 1) == -1 || dup2(pipe_fd_sat_grep[0], 0) == -1) {
	            handle_error("Grep pipe dup");
	        }


			char *args[] = {"grep", "-o", "-e", "Number of .*[0-9]\\+", "-e", "CPU time.*", "-e", ".*SATISFIABLE", NULL};
			if (execvp("grep", args) < 0) {
	        	handle_error("Grep exec");
	        }
	        
	        close(pipe_fd_sat_grep[0]); //Cierro lo que no uso
			close(pipe_fd_grep_slave[1]);
	        exit(0);

		} else if(forkGrep == -1) {
			handle_error("Forking for grep");
		}

		//Se cierra lo que no se usa
		close(pipe_fd_sat_grep[0]);
		close(pipe_fd_grep_slave[1]);


		//Comenzamos a construir el csv output
		strcat(output, token); //Primer columna es el nombre del archivo
		strcat(output, ",");

		size_t auxSize = 50;
		FILE * fp = fdopen(pipe_fd_grep_slave[0], "r");
		char * aux = malloc(auxSize); //Aquí va guardando getline
		while(getline(&aux, &auxSize, fp) > 0) {
			strcat(output, aux);
			output[strlen(output)-1] = ',';
		}

		pid_t slave_pid = getpid();  //Finalmente añadimos pid
		char str_for_pid[15];
		sprintf(str_for_pid, "%d", slave_pid);
		strcat(output, "Slave PID: "); //Amigable para usuario
		strcat(output, str_for_pid);
		strcat(output, "\n");
		strcat(output, "----------------\n");
		fclose(fp);

		close(pipe_fd_grep_slave[0]);

		int intresult;
		waitpid(forkMinisat, &intresult, WUNTRACED); //Esperamos a que termine el minisat
		waitpid(forkGrep, &intresult, WUNTRACED); //Esperamos a que termine el grep


		token = strtok(NULL, ",");
	}


}
