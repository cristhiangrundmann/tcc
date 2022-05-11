.phony: default clean

default: highlight.out

test.out: parser.o table.o lexer.o
	clang++ -g parser.o table.o lexer.o -o test.out -std=c++17

parser.o: parser.cpp parser.hpp lexer.hpp table.hpp
	clang++ -c -g parser.cpp -o parser.o -std=c++17

table.o: table.cpp table.hpp
	clang++ -c -g table.cpp -o table.o -std=c++17

lexer.o: lexer.cpp lexer.hpp table.hpp
	clang++ -c -g lexer.cpp -o lexer.o -std=c++17

highlight.out: highlight.o parser.o table.o lexer.o
	clang++ -Wall -Wextra -Wpedantic -g highlight.o parser.o table.o lexer.o -o highlight.out `wx-config --libs all` -std=c++17

highlight.o: highlight.cpp parser.hpp table.hpp lexer.hpp
	clang++ -c -Wall -Wextra -Wpedantic -g highlight.cpp -o highlight.o `wx-config --cppflags` -std=c++17

clean:
	-@rm *out *o