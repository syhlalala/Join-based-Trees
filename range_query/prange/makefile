CC = g++ 
CFLAGS = -O2 -DNDEBUG -I../c++ -mcx16 -g -march=native -fcilkplus -std=c++11 -Wall

all: 	test

test:	test.cpp prange.h
	$(CC) $(CFLAGS) test.cpp -o test

clean:
	rm -f test
