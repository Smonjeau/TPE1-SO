#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
	//No se hace define pues en getline se necesita puntero
	size_t bufferSize = 1024;

	//Slave recibe por pipe direcciones de los archivos

	char * buffer = (char *) malloc(bufferSize * sizeof(char));
	
	if(buffer == NULL)
	{
		perror("No se pudo reservar espacio para buffer");
		exit(-1);
	}
	

	while(getline(&buffer, &bufferSize, 0) != -1 /*&& Condicion de que no recibí señal para terminar */)
	{
		//Tengo en buffer lo que me mando master para procesar
		//algo anda mal xd
	}
	
	exit(0); //Exito
}