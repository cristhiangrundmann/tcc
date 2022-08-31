CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g -I../imgui -I../compiler
LIBS = -lglfw -ldl -lGL -lGLEW -lm
COMPILER = ../compiler/compiler.a
IMGUI = ../imgui/imgui.a

.phony: compiler imgui clean

prog.out: main.o compiler imgui
	clang++ $(CPPFLAGS) main.o $(IMGUI) $(COMPILER) -o prog.out $(LIBS)

compiler:
	cd ../compiler && $(MAKE)

imgui:
	cd ../compiler && $(MAKE)

%.o: %.cpp
	clang++ $(CPPFLAGS) -c -o $@ $<

clean:
	-rm *.o *.out