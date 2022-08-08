CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -O3 -I../imgui -I../compiler
LIBS = -lglfw -ldl -lGL -lGLEW
COMPILER = ../compiler/compiler.a
IMGUI = ../imgui/imgui.a

.phony: compiler imgui

prog.out: main.o compiler imgui
	clang++ $(CPPFLAGS) main.o $(IMGUI) $(COMPILER) -o prog.out $(LIBS)

compiler:
	cd ../compiler/; make

imgui:
	cd ../imgui/; make

%.o: %.cpp
	clang++ $(CPPFLAGS) -c -o $@ $<