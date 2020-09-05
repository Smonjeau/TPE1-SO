#ifndef __MASTER_VIEW__H
#define __MASTER_VIEW__H

#define SHM_SIZE 1024                         // Master-view shared memory size in bytes

#define SEM_READ_BYTES "/read_bytes"          // Semaphore whose value is the available bytes for read
#define SEM_WRITE_BYTES "/write_bytes"        // Semaphore whose value is the available bytes for write

#define EOT 4                                 // Character used to signal view that there is no more data

#endif