CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -O3

.phony: clean

imgui.a: imgui_demo.o imgui_draw.o backends/imgui_impl_glfw.o backends/imgui_impl_opengl3.o imgui_tables.o imgui_widgets.o imgui.o
	-rm imgui.a
	ar -crv imgui.a `ls *.o backends/*.o`

%.o: %.cpp
	clang++ -I./ $(CPPFLAGS) -c -o $@ $<

clean:
	-rm *.o *.a