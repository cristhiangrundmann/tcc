CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g -I../imgui -I../compiler
LIBS = -lglfw -ldl -lGL -lGLEW
COMPILER = ../compiler/compiler.a
IMGUI = ../imgui/imgui.a

.phony: clean

prog.out: main.o $(COMPILER) $(IMGUI)
	clang++ $(CPPFLAGS) main.o $(IMGUI) $(COMPILER) -o prog.out $(LIBS)

$(COMPILER):
	make -C ../compiler

$(IMGUI):
	make -C ../imgui

%.o: %.cpp
	clang++ $(CPPFLAGS) -c -o $@ $<

clean:
	-rm *.o *.out