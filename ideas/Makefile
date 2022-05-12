CPPFLAGS=-Wall -Wextra -Wpedantic -std=c++17 -O3

.phony: default clean

default: highlight.out

test.out: parser.o table.o lexer.o
	clang++ parser.o table.o lexer.o -o test.out $(CPPFLAGS)

parser.o: parser.cpp parser.hpp lexer.hpp table.hpp
	clang++ -c parser.cpp -o parser.o $(CPPFLAGS)

table.o: table.cpp table.hpp
	clang++ -c table.cpp -o table.o $(CPPFLAGS)

lexer.o: lexer.cpp lexer.hpp table.hpp
	clang++ -c lexer.cpp -o lexer.o $(CPPFLAGS)

highlight.out: highlight.o parser.o table.o lexer.o
	clang++ highlight.o parser.o table.o lexer.o -o highlight.out `wx-config --libs all` $(CPPFLAGS)

highlight.o: highlight.cpp parser.hpp table.hpp lexer.hpp
	clang++ -c highlight.cpp -o highlight.o `wx-config --cppflags` $(CPPFLAGS)

clean:
	-@rm *out *o