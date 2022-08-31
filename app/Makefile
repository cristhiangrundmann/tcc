CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -O3 -Iimgui -Icompiler -Istb_image
LIBS = -lglfw -ldl -lGL -lGLEW -lm
CC = clang++

.phony: clean

all: prog.out
	./prog.out

%.o: %.cpp %.hpp
	$(CC) $(CPPFLAGS) -c -o $@ $<

COMPILER += compiler/compiler.o
COMPILER += compiler/parser.o
COMPILER += compiler/lexer.o
COMPILER += compiler/table.o

compiler.a: $(COMPILER)
	$(AR) -crv compiler.a $(COMPILER)

compiler/compiler.o: compiler/parser.hpp compiler/lexer.hpp compiler/table.hpp
compiler/parser.o: compiler/lexer.hpp compiler/table.hpp
compiler/lexer.o: compiler/table.hpp

IMGUI += imgui/imgui_demo.o
IMGUI += imgui/imgui_draw.o
IMGUI += imgui/imgui_tables.o
IMGUI += imgui/imgui_widgets.o
IMGUI += imgui/imgui.o
IMGUI += imgui/backends/imgui_impl_glfw.o
IMGUI += imgui/backends/imgui_impl_opengl3.o

imgui.a: $(IMGUI)
	$(AR) -crv imgui.a $(IMGUI)

STB_IMAGE += stb_image/stb_image.o

stb_image.o: $(STB_IMAGE)
	cp $(STB_IMAGE) stb_image.o

test/main.o: compiler/compiler.hpp

prog.out: compiler.a imgui.a stb_image.o test/main.o
	$(CC) $(CPPFLAGS) test/main.o compiler.a imgui.a stb_image.o -o prog.out $(LIBS)

clean:
	-rm $(IMGUI) imgui.a
	-rm $(COMPILER) compiler.a
	-rm $(STB_IMAGE) stb_image.o
	-rm test/main.o
	-rm prog.out