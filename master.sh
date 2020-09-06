#!/bin/bash
make clean && make -s
rm /dev/shm/sem.read_bytes && rm /dev/shm/sem.write_bytes
./master.out a b c d e f g h i