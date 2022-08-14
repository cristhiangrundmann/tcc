#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "compiler.hpp"

#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace tcc;
using std::vector;
using std::unique_ptr;
using std::make_unique;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 460";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		printf("Error: %s\n", glewGetErrorString(err));
	}

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", 18);
    ImFont * f2 = io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", 24);

    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


		{
			ImGui::Begin("Text Editor");
			static char text[1024 * 16];
            ImGui::PushFont(f2);
            ImGui::InputTextMultiline("src", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_AllowTabInput);
			ImGui::PopFont();

            static std::string error;
            static const char *msg = "OK";
            
            if(ImGui::Button("Compile"))
            {
                static Compiler cmp;
                cmp = Compiler();
                try
                {
                    cmp.parseProgram(text);
                    msg = "OK";

                    Table *t = cmp.table->procString("f", true);
                    if(t->length != 1 || t->type != TokenType::FUNCTION) throw std::string("`f` is not defined");
                    //if(t->argsIndex == -1) throw std::string("Error ?");
                    if(cmp.argList[t->argsIndex].size() != 1) throw std::string("`f` must be a single nameiable function");

                }
                catch(std::string err)
                {
                   error = err;
                   msg = error.c_str();
                }
            }

            ImGui::Text("Status: %s", msg);


            ImGui::End();
		}

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}