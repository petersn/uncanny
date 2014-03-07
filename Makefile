
CPPFLAGS=-Wall -O3 -g -ffast-math -std=c++0x

ucan_test: ucan_test.o uncanny.o Makefile
	g++ -o $@ $(CPPFLAGS) $< uncanny.o

.PHONY: clean
clean:
	rm -f ucan_test *.o

