CPPFLAGS=-Wall -Wextra -Wpedantic -Werror -std=c++17 -O3

.phony: default clean

default: latex.out highlight.out

test.out: parser.o table.o lexer.o
	clang++ parser.o table.o lexer.o -o test.out $(CPPFLAGS)

parser.o: parser.cpp parser.hpp lexer.o
	clang++ -c parser.cpp -o parser.o $(CPPFLAGS)

table.o: table.cpp table.hpp
	clang++ -c table.cpp -o table.o $(CPPFLAGS)

lexer.o: lexer.cpp lexer.hpp table.o
	clang++ -c lexer.cpp -o lexer.o $(CPPFLAGS)

highlight.out: highlight.o
	clang++ highlight.o parser.o table.o lexer.o -o highlight.out `wx-config --libs all` $(CPPFLAGS)

highlight.o: highlight.cpp parser.o style.hpp
	clang++ -c highlight.cpp -o highlight.o `wx-config --cppflags` $(CPPFLAGS)

latex.out: latex.o
	clang++ latex.o parser.o table.o lexer.o -o latex.out $(CPPFLAGS)

latex.o: latex.cpp parser.o style.hpp
	clang++ -c latex.cpp -o latex.o $(CPPFLAGS)

clean:
	-@rm *out *o