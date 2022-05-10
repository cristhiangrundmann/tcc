.phony: default highlight clean

default: test.out

test.out: parser.o table.o lexer.o
	clang++ -g parser.o table.o lexer.o -o test.out -std=c++17

parser.o: parser.cpp parser.hpp lexer.hpp table.hpp
	clang++ -c -g parser.cpp -o parser.o -std=c++17

table.o: table.cpp table.hpp
	clang++ -c -g table.cpp -o table.o -std=c++17

lexer.o: lexer.cpp lexer.hpp table.hpp
	clang++ -c -g lexer.cpp -o lexer.o -std=c++17

highlight:
	clang++ -Wall -Wextra -Wpedantic -O3 highlight.cpp -o highlight.out `wx-config --libs all --cppflags` -std=c++17
	./highlight.out

clean:
	-@rm *out *o