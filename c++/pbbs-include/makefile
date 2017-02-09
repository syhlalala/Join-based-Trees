ifdef CLANG
CC = clang++ 
CFLAGS = -mcx16 -O2 -I../c++ -fcilkplus -ldl -std=c++11 -march=native
else
CC = g++
# works with g++ (GCC) 4.9.0 (compiled for cilkplus)
CFLAGS = -mcx16 -O2 -Wall -fcilkplus -std=c++11 -march=native
endif

time_tests:	time_tests.cpp time_operations.h
	$(CC) $(CFLAGS) time_tests.cpp -o time_tests

all:	time_tests 

clean:
	rm -f time_tests
