
ifdef CLANG
CC = clang++ 
CFLAGS = -mcx16 -O2 -I../c++ -fcilkplus -ldl -std=c++11 -march=native
else
CC = g++
# works with g++ (GCC) 5.4.1
CFLAGS = -O2 -I../c++ -mcx16 -march=native -fcilkplus -std=c++11 -Wall
endif

intervalTree:	intervalTree.cpp
	$(CC) $(CFLAGS) intervalTree.cpp -o intervalTree

all:	intervalTree.cpp

clean:
	rm -f test intervalTree
