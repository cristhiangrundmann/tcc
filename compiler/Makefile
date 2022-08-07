CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -O3

compiler.a: lexer.o parser.o table.o
	-rm compiler.a
	ar -crv compiler.a lexer.o parser.o table.o

headers: table.hpp lexer.hpp parser.hpp
	touch headers

%.o: %.cpp headers
	clang++ $(CPPFLAGS) -c -o $@ $<