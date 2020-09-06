# TPE 1 - Sistemas Operativos - Grupo 10

## Instalación

Se recomienda utilizar el entorno de docker provisto por la cátedra.
```bash
docker pull agodio/itba-so:1.0
```

Es requisito contar con el software *minisat*. Para ello correr la instrucción:

```bash
apt-get install minisat
```

Una vez clonado el repositorio posicionarse dentro del directorio y ejecutar:
```bash
make all
```

## Instrucciones de uso

Ejecutar "./master.out [params]" donde [params] es una secuencia de archivos con extensión cnf. En caso de ejecutar master de esta manera, se imprimirá en la salida estándar el nombre de la zona de memoria compartida con la view (shmName). Para ejecutar el proceso vista, escribir "./view.out shmName".
La otra manera de ejecutar los procesos es mediante un pipe: "./master.out [params] $|$ ./view". 
