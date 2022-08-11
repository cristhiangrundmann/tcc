CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -O3

.phony: clean

compiler.a: table.o lexer.o parser.o compiler.o
	-rm compiler.a
	ar -crv compiler.a table.o lexer.o parser.o compiler.o

headers: table.hpp lexer.hpp parser.hpp compiler.hpp
	touch headers

%.o: %.cpp headers
	clang++ $(CPPFLAGS) -c -o $@ $<

clean:
	-rm *.o *.a *.out