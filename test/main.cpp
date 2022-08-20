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

Compiler cmp;

unsigned int compileShader(int type, const char *source)
{
    unsigned int i = glCreateShader(type);
    glShaderSource(i, 1, &source, NULL);
    glCompileShader(i);
    int success;
    static char infoLog[512];
    glGetShaderiv(i, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(i, 512, NULL, infoLog);
        throw std::string("Cannot compile shader: ") + infoLog;
    }
    return i;
}

unsigned int linkProgram(int *shaders, int size)
{
    unsigned int i = glCreateProgram();
    for(int k = 0; k < size; k++)
        glAttachShader(i, shaders[k]);
    glLinkProgram(i);
    int success;
    static char infoLog[512];
    glGetProgramiv(i, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(i, 512, NULL, infoLog);
        throw std::string("Cannot link shader: ") + infoLog;
    }
    return i;
}

const char *vertexShaderSource = 
"#version 460 core\n"
"layout (location = 0) in vec2 aPos;\n"
"out vec2 pos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
"   pos = aPos;\n"
"}\n\0";

/*const char *fragmentShaderSource = 
"#version 460 core\n"
"out vec4 FragColor;\n"
"in vec2 pos;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(, 0, 0, 1);\n"
"}\n\0";*/

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

    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    printf("Framebuffer is not complete!");
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float vertices[] =
    {
        -1.0f, -1.0f,
        +1.0f, -1.0f,
        +1.0f, +1.0f,

        -1.0f, -1.0f,
        -1.0f, +1.0f,
        +1.0f, +1.0f,
    }; 

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		{

            static bool use_work_area = true;
            static ImGuiWindowFlags flags = 
                ImGuiWindowFlags_NoDecoration | 
                ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoSavedSettings;

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
            ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);
			ImGui::Begin("Function Test", nullptr, flags);
			static char text[1024 * 16];
            ImGui::PushFont(f2);
            ImGui::InputTextMultiline("src", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_AllowTabInput);
			ImGui::PopFont();

            static std::string error;
            static const char *msg = "OK";
            
            if(ImGui::Button("Compile"))
            {
                cmp = Compiler();
                try
                {
                    cmp.parseProgram(text);
                    msg = "OK";
                    std::stringstream s1, s2;
                    cmp.compile(s1);
                    cmp.header(s2);

                    for(Obj &o : cmp.objects)
                    {
                        if(o.type == cmp.function)
                        {
                            if(!o.compSub[0]) break;
                            if(cmp.argList[o.name->argIndex].size() != 2) break;

                            std::stringstream s;

                            s << "#version 460 core\nin vec2 pos;\nout vec4 FragColor;\n" << s2.str() << s1.str();
                            s << "void main()\n{\n";
                            s << "float b = 0.01;\n";
                            s << "float d = abs(" << "F" << o.name->getString() << "(pos.x/0.9, -pos.y/0.9));\n";
                            s << "float red = 0;\n";
                            s << "if(d < b) red = (b-d)/b;\n";
                            s << "FragColor = vec4(red, red*(pos.x+1)/2, red*(pos.y+1)/2, 1);\n}\n";

                            printf("%s\n", s.str().c_str());

                            int fs = compileShader(GL_FRAGMENT_SHADER, s.str().c_str());
                            int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
                            int shaders[2] = {fs, vs};
                            int sp = linkProgram(shaders, 2);

                            glUseProgram(sp);
                            glDisable(GL_DEPTH_TEST);
                            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                            glBindVertexArray(VAO);
                            glViewport(0, 0, 512, 512);
                            glDrawArrays(GL_TRIANGLES, 0, 6);
                            glBindFramebuffer(GL_FRAMEBUFFER, 0);
                            glDeleteShader(fs);
                            glDeleteShader(vs);
                            glDeleteProgram(sp);
                            break;
                        }
                    }
                }
                catch(std::string err)
                {
                   error = err;
                   msg = error.c_str();
                }
            }

            ImGui::Text("Status: %s", msg);

            for(Obj &o : cmp.objects)
            {
                std::vector<Subst> subs;
                if(o.type == cmp.param)
                {
                    ImGui::PushID(o.name);
                    for(int i = 0; i < (int)o.intervals.size(); i++)
                    {
                        ImGui::PushID(i);
                        float min = cmp.calculate(o.intervals[i].compSub[0], subs);
                        float max = cmp.calculate(o.intervals[i].compSub[1], subs);
                        o.intervals[i].min = min;
                        o.intervals[i].max = max;
                        std::string str = o.name->getString();
                        if(o.intervals.size() > 1)
                        {
                            str += "_";
                            str += std::to_string(i+1);
                        }
                        ImGui::SliderFloat(str.c_str(), &o.intervals[i].number, min, max, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::Image((void*)(intptr_t)texture, ImVec2(512, 512));
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