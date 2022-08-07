CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -O3

compiler.a: lexer.o parser.o table.o
	ar -cqv compiler.a `ls *.o`

headers: table.hpp lexer.hpp parser.hpp
	touch headers

%.o: %.cpp headers
	clang++ $(CPPFLAGS) -c -o $@ $<