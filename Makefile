
FLAGS=-Wall -O3 -g -ffast-math -std=c++11

ucan_test: ucan_test.o uncanny.o Makefile
	g++ -o $@ $(CFLAGS) $< uncanny.o

.PHONY: clean
clean:
	rm -f ucan_test *.o

