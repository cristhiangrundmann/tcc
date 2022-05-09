phony: default highlight

default: highlight

highlight:
	clang++ -Wall -Wextra -Wpedantic -O3 highlight.cpp -o highlight.out `wx-config --libs all --cppflags` -std=c++17
	./highlight.out