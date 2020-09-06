#define MAX_MESSAGE_LEN 1000

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_SIZE_FOR_MINISAT_OUTPUT 2048
#define MAX_SIZE_FOR_GREP_OUTPUT 1024



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
        	
            char output[MAX_MESSAGE_LEN]={0};
            process(input, output, slave_id);

            // Return the result            
            write(wr_fd, output, MAX_MESSAGE_LEN);
        }

        while(strcmp(input, "ACK") != 0){
            read(rd_fd, input, MAX_MESSAGE_LEN);
        }

    }

    exit(0);
    
}


void process(char *input, char *outputForMaster, int slave_id) {
	// Split the input and process each path, then return all answers in CSV format

    //printf("@S%d: %s\n", slave_id, input);
    int pipe_fd_sat_grep[2];
    int pipe_fd_grep_slave[2];
    //char * outputForMaster = malloc(MAX_SIZE_FOR_GREP_OUTPUT);
    pid_t forkMinisat, forkGrep;  

	char * token = malloc(100 * sizeof(char)); // token almacena nombre de archivo cnf
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
			// printf("proximo: %s\n", token);
			
	        if (execv("/usr/bin/minisat", args) < 0) {
	        	perror("exec");
	            exit(EXIT_FAILURE);
	        }




			close(pipe_fd_sat_grep[1]);
	        exit(0);

		} else if(forkMinisat == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}

		close(pipe_fd_sat_grep[1]); //La escritura acá no la necesito
		
		

		
		

		//Pipe para comunicar grep y slave
		if(pipe(pipe_fd_grep_slave) == -1) {	
			perror("pipe");
			exit(EXIT_FAILURE);
		}		



		forkGrep = fork();
		if(forkGrep == 0) { //Proceso que ejcuta grep para filtrar salida del minisat		
			close(pipe_fd_sat_grep[1]); //Cierro lo que no uso
			close(pipe_fd_grep_slave[0]); //Cierro lo que no uso

			if (dup2(pipe_fd_grep_slave[1], 1) == -1 || dup2(pipe_fd_sat_grep[0], 0) == -1) {
	            perror("dup");
	            exit(EXIT_FAILURE);
	        }
	        


			char *args[] = {"grep", "-o", "-e", "Number of .*[0-9]\\+", "-e", "CPU time.*", "-e", ".*SATISFIABLE", NULL};
			if (execv("/bin/grep", args) < 0) {
	        	perror("exec");
	            exit(EXIT_FAILURE);
	        }
	        
	        close(pipe_fd_sat_grep[0]); //Cierro lo que no uso
			close(pipe_fd_grep_slave[1]);
	        exit(0);

		} else if(forkGrep == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		//Se cierra lo que no se usa
		close(pipe_fd_sat_grep[0]);
		close(pipe_fd_grep_slave[1]);


		//Comenzamos a cosntruir el csv output
		strcat(outputForMaster, token); //Primer columna es el nombre del archivo
		strcat(outputForMaster, ",");

		size_t auxSize = 50;
		FILE * fp = fdopen(pipe_fd_grep_slave[0], "r");
		char * aux = malloc(auxSize); //Aquí va guardando getline
		while(getline(&aux, &auxSize, fp) > 0) {
			strcat(outputForMaster, aux);
			outputForMaster[strlen(outputForMaster)-1] = ',';
		}

		pid_t slave_pid = getpid();  //Finalmente añadimos pid
		char str_for_pid[15];
		sprintf(str_for_pid, "%d", slave_pid);
		strcat(outputForMaster, "Slave PID: "); //Amigable para usuario
		strcat(outputForMaster, str_for_pid);
		strcat(outputForMaster, "\n");
		fclose(fp);

		close(pipe_fd_grep_slave[0]);

		int intresult;
		waitpid(forkMinisat, &intresult, WUNTRACED); //Esperamos a que termine el minisat
		waitpid(forkGrep, &intresult, WUNTRACED); //Esperamos a que termine el grep

		// printf(outputForMaster);

		token = strtok(NULL, ",");
	}


}