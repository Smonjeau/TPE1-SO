#ifndef __MASTER_VIEW__H
#define __MASTER_VIEW__H


#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#define SHM_SIZE 1024                         // Master-view shared memory size in bytes
#define SEM_READ_BYTES "/read_bytes"          // Semaphore whose value is the available bytes for read
#define SEM_WRITE_BYTES "/write_bytes"        // Semaphore whose value is the available bytes for write

#endif