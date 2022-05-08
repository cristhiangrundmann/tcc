all: prog.out
	./prog.out

prog.out: main.o
	clang++ -Wall -Wextra -Wpedantic -Werror -O3 main.o -o prog.out `wx-config --libs all` -std=c++17

main.o: main.cpp
	clang++ -c -O3 main.cpp -o main.o `wx-config --cppflags` -std=c++17