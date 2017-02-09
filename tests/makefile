
ifdef CLANG
CC = clang++ 
CFLAGS = -mcx16 -O2 -I../c++ -fcilkplus -ldl -std=c++11 -march=native
else
CC = g++
# works with g++ (GCC) 5.4.1
CFLAGS = -O2 -I../c++ -mcx16 -march=native -fcilkplus -std=c++11 -Wall
endif

release:		testParallel.cpp
	$(CC) $(CFLAGS) -DNDEBUG testParallel.cpp -o testParallel

debug:		testParallel.cpp
	$(CC) $(CFLAGS) testParallel.cpp -o testParallel

testParallel:		testParallel.cpp
	$(CC) $(CFLAGS) -DNDEBUG testParallel.cpp -o testParallel

unit_tests:	unit_tests.cpp
	$(CC) $(CFLAGS) unit_tests.cpp -o unit_tests

test:	test.cpp
	$(CC) $(CFLAGS) test.cpp -o test

all:	test testParallel unit_tests

clean:
	rm -f test testParallel unit_tests
