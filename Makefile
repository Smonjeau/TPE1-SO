%.out: %.c
	gcc -Wall -std=c99 -pedantic -D_POSIX_C_SOURCE -D_XOPEN_SOURCE=500 -lrt -pthread -g -o $@ $<

all: master.out slave.out view.out

clean:
	rm -rf *.out