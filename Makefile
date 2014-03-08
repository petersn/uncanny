
CPPFLAGS=-Wall -O3 -g -ffast-math -std=c++0x

all: ucan_test query_test

.PHONY: objects
objects: uncanny.o uncanny_query.o

query_test: objects Makefile
	g++ -o $@ $<

ucan_test: ucan_test.o uncanny.o Makefile
	g++ -o $@ $(CPPFLAGS) $< uncanny.o

.PHONY: clean
clean:
	rm -f ucan_test *.o

