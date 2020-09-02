%.out: %.c
	gcc -o $@ $<

all: master.out slave.out