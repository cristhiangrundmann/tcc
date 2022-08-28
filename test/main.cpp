#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "compiler.hpp"

#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

using namespace tcc;
using namespace glm;
using std::vector;
using std::unique_ptr;
using std::make_unique;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Compiler *cmp{};

void draw2(Obj &o)
{
    if(!o.program.ID) return;
    
    if(o.type == cmp->curve)
    {
        glUseProgram(o.program.ID);
        glBindVertexArray(o.array.ID);
        glDrawArrays(GL_LINE_STRIP, 0, o.intervals[0].number);
    }

    if(o.type == cmp->surface)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUseProgram(o.program.ID);
        glBindVertexArray(o.array.ID);
        uint count = o.intervals[0].number*o.intervals[1].number*6;
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if(o.type == cmp->point)
    {
        glUseProgram(o.program.ID);
        glBindVertexArray(cmp->line.ID);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    if(o.type == cmp->vector)
    {
        glUseProgram(o.program.ID);
        glBindVertexArray(cmp->line.ID);
        glDrawArrays(GL_LINES, 0, 2);
    }
}

void draw(Obj &o)
{
    if(o.grids.size() == 0)
    {
        draw2(o);
        return;
    }

    struct State
    {
        uint index{};
        Interval g{};
    };

    std::vector<State> states;

    glBindBuffer(GL_UNIFORM_BUFFER, cmp->block.ID);
    for(uint s = 0; s < o.grids.size(); s++)
    {
        Obj &o2 = cmp->objects[o.grids[s]];
        for(Interval &i : o2.intervals)
        {
            glBufferSubData(GL_UNIFORM_BUFFER, i.offset, 4, &i.min);
            states.push_back({0, i});
        }
    }

    while(true)
    {
        draw2(o);

        bool carry = true;

        for(uint s = 0; s < states.size(); s++)
        {
            Interval &g = states[s].g;
            states[s].index++;
            if(states[s].index >= g.number)
                states[s].index = 0;
            else
                carry = false;
            float x = g.min + (g.max - g.min)*states[s].index/(g.number-1);
            glBufferSubData(GL_UNIFORM_BUFFER, g.offset, 4, &x);
            if(!carry) break;
        }

        if(carry) break;
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

int main(int, char**)
{
    cmp = new Compiler;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 460";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "tcc", NULL, NULL);
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

    glEnable(GL_LINE_SMOOTH);
    glLineWidth(10);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(10);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		{
			ImGui::Begin("Function Test");
			static char text[1024 * 16];
            ImGui::PushFont(f2);
            ImGui::InputTextMultiline("src", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_AllowTabInput);
			ImGui::PopFont();

            static std::string error;
            static const char *msg = "OK";
    
            if(ImGui::Button("Compile"))
            {
                if(cmp->compiled)
                {
                    delete cmp;
                    cmp = new Compiler;
                }
                try
                {
                    cmp->compile(text);
                    msg = "OK";
                }
                catch(std::string err)
                {
                   error = err;
                   msg = error.c_str();
                   delete cmp;
                   cmp = new Compiler;
                }
            }

            ImGui::Text("Status: %s", msg);
            ImGui::End();
		}

        static float _t = 0;
        float _t2 = ImGui::GetTime();

        if(cmp->compiled)
        {
            ImGui::Begin("Settings");
            glBindBuffer(GL_UNIFORM_BUFFER, cmp->block.ID);
            mat4 id = identity<mat4>();
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &id);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, cmp->frame.ID);
            glViewport(0, 0, 512, 512);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            for(Obj &o : cmp->objects)
            {
                std::vector<Subst> subs;
                if(o.type == cmp->param)
                {
                    ImGui::PushID(o.name);
                    for(int i = 0; i < (int)o.intervals.size(); i++)
                    {
                        ImGui::PushID(i);
                        std::string str = o.name->str;
                        if(o.intervals.size() > 1)
                        {
                            str += "_";
                            str += std::to_string(i+1);
                        }
                        float cur = o.intervals[i].number;
                        ImGui::Checkbox("Animate", &o.intervals[i].animate);
                        ImGui::SameLine();
                        if(o.intervals[i].animate)
                        {
                            float t = o.intervals[i].number + (_t2 - _t);
                            if(t > o.intervals[i].max) t = o.intervals[i].min;
                            o.intervals[i].number = t;
                        }
        
                        ImGui::SliderFloat(str.c_str(), &o.intervals[i].number, 
                            o.intervals[i].min, o.intervals[i].max, "%.3f", ImGuiSliderFlags_AlwaysClamp);

                        if(cur != o.intervals[i].number)
                        {
                            glBindBuffer(GL_UNIFORM_BUFFER, cmp->block.ID);
                            glBufferSubData(GL_UNIFORM_BUFFER, o.intervals[i].offset, 4, &o.intervals[i].number);
                        }
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }

                if(o.type == cmp->curve ||
                o.type == cmp->surface ||
                o.type == cmp->point ||
                o.type == cmp->vector)
                {
                    Color col = {o.col[0], o.col[1], o.col[2], o.col[3]};
                    ImGui::PushID(o.name);
                    ImGui::ColorEdit3(o.name->str.c_str(), o.col, ImGuiColorEditFlags_NoInputs);
                    ImGui::PopID();

                    if(col[0] != o.col[0] ||
                    col[1] != o.col[1] ||
                    col[2] != o.col[2] ||
                    col[3] != o.col[3])
                    {
                        glUseProgram(o.program.ID);
                        glUniform4f(0, o.col[0], o.col[1], o.col[2], o.col[3]);
                    }

                    draw(o);
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            ImGui::End();
        }

        _t = _t2;


        if(cmp->compiled)
        {
            ImGui::Begin("Output");
            ImGui::Image((void*)(intptr_t)cmp->frame.textures[0]->ID, ImVec2(512, 512));
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

    delete cmp;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}