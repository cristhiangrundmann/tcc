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

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Compiler *cmp{};
Texture *tex{};

vec2 rotate(Obj &o, vec2 pos, vec2 vec, float angle)
{
    std::vector<Subst> subs;
    int index = o.name->argIndex;
    subs.push_back({cmp->argList[index][0], nullptr, pos.x});
    subs.push_back({cmp->argList[index][1], nullptr, pos.y});
    float E = cmp->calculate(o.compSub[3], subs);
    float F = cmp->calculate(o.compSub[4], subs);
    float G = cmp->calculate(o.compSub[5], subs);

    float R = std::sqrt(E*G-F*F);

    return vec2
    (
        vec.x*std::cos(angle) - (vec.x*F+vec.y*G)/R*std::sin(angle),
        vec.y*std::cos(angle) + (vec.x*E+vec.y*F)/R*std::sin(angle)
    );
}

float length(Obj &o, vec2 pos, vec2 vec)
{
    std::vector<Subst> subs;
    int index = o.name->argIndex;
    subs.push_back({cmp->argList[index][0], nullptr, pos.x});
    subs.push_back({cmp->argList[index][1], nullptr, pos.y});
    float E = cmp->calculate(o.compSub[3], subs);
    float F = cmp->calculate(o.compSub[4], subs);
    float G = cmp->calculate(o.compSub[5], subs);

    return std::sqrt(vec.x*vec.x*E + 2*vec.x*vec.y*F + vec.y*vec.y*G);
}

vec2 accel(Obj &o, vec2 pos, vec2 vec)
{
    std::vector<Subst> subs;
    int index = o.name->argIndex;
    subs.push_back({cmp->argList[index][0], nullptr, pos.x});
    subs.push_back({cmp->argList[index][1], nullptr, pos.y});

    float E = cmp->calculate(o.compSub[3], subs);
    float F = cmp->calculate(o.compSub[4], subs);
    float G = cmp->calculate(o.compSub[5], subs);
    float Eu = cmp->calculate(o.compSub[6], subs);
    float Ev = cmp->calculate(o.compSub[7], subs);
    float Fu = cmp->calculate(o.compSub[8], subs);
    float Fv = cmp->calculate(o.compSub[9], subs);
    float Gu = cmp->calculate(o.compSub[10], subs);
    float Gv = cmp->calculate(o.compSub[11], subs);
    float R = E*G-F*F;

    float A = (Gu/2-Fv)*vec.y*vec.y-(Eu/2*vec.x+Ev*vec.y)*vec.x;
    float B = (Ev/2-Fu)*vec.x*vec.x-(Gv/2*vec.y+Gu*vec.x)*vec.y;

    return mat2(G, -F, -F, E)*vec2(A, B)/R;
}

void step(Obj &o, vec2 &pos, vec2 &vec, float h)
{
    vec2 k1_pos = h*vec;
    vec2 k1_vec = h*accel(o, pos, vec);
    vec2 k2_pos = h*(vec+k1_vec/2.0f);
    vec2 k2_vec = h*accel(o, pos+k1_pos/2.0f, vec+k1_vec/2.0f);
    vec2 k3_pos = h*(vec+k2_vec/2.0f);
    vec2 k3_vec = h*accel(o, pos+k2_pos/2.0f, vec+k2_vec/2.0f);
    vec2 k4_pos = h*(vec+k3_vec);
    vec2 k4_vec = h*accel(o, pos+k3_pos, vec+k3_vec);
    pos += (k1_pos + 2.0f*k2_pos + 2.0f*k3_pos + k4_pos)/6.0f;
    vec += (k1_vec + 2.0f*k2_vec + 2.0f*k3_vec + k4_vec)/6.0f;
}

void draw2(Obj &o)
{
    if(!o.program[0].ID) return;
    
    if(o.type == cmp->curve)
    {
        glUseProgram(o.program[0].ID);
        glBindVertexArray(o.array.ID);
        glDrawArrays(GL_LINE_STRIP, 0, o.intervals[0].number);
    }

    if(o.type == cmp->surface)
    {
        glUseProgram(o.program[0].ID);
        glBindVertexArray(o.array.ID);
        uint count = o.intervals[0].number*o.intervals[1].number*6;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->ID);

        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, cmp->frame.ID);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);

        glUseProgram(o.program[1].ID);
        glBindFramebuffer(GL_FRAMEBUFFER, cmp->uvFrame.ID);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);

        ImGui::PushID(o.name);
        ImGui::Begin("GEO");

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("btn2", ImVec2(cmp->frameSize.width, cmp->frameSize.height), ImGuiButtonFlags_MouseButtonLeft);
        bool held = ImGui::IsItemActive();
        bool hover = ImGui::IsItemHovered();

        if(hover)
        {
            ImGuiIO& io = ImGui::GetIO();
            float w = ImGui::IsKeyDown('W') ? 1 : 0;
            float s = ImGui::IsKeyDown('S') ? 1 : 0;
            float a = ImGui::IsKeyDown('A') ? 1 : 0;
            float d = ImGui::IsKeyDown('D') ? 1 : 0;
            float q = ImGui::IsKeyDown('Q') ? 1 : 0;
            float e = ImGui::IsKeyDown('E') ? 1 : 0;
            float z = ImGui::IsKeyDown('Z') ? 1 : 0;
            float x = ImGui::IsKeyDown('X') ? 1 : 0;

            o.zoom += (x-z)*io.DeltaTime;

            float l = length(o, o.center, o.X);
            o.X *= o.zoom/l;
            o.X = rotate(o, o.center, o.X, o.ori*(q-e)*io.DeltaTime);
            step(o, o.center, o.X, (d-a)*io.DeltaTime);
            o.Y = rotate(o, o.center, o.X, o.ori*Parser::CPI/2);
            step(o, o.center, o.Y, (s-w)*io.DeltaTime);
            o.X = rotate(o, o.center, o.Y, -o.ori*Parser::CPI/2);
        }

        glUseProgram(o.program[2].ID);
    
        glUniform2f(0, o.center.x, o.center.y);
        glUniform2f(1, o.X.x, o.X.y);
        glUniform2f(2, o.Y.x, o.Y.y);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->ID);
        glBindVertexArray(cmp->quad.ID);
        glBindFramebuffer(GL_FRAMEBUFFER, o.frame.ID);
        glEnable(GL_TEXTURE_2D);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        ImGui::SetCursorScreenPos(pos);
        ImGui::Image((void*)(intptr_t)o.frame.textures[1]->ID, ImVec2(cmp->frameSize.width, cmp->frameSize.height));
        ImGui::End();
        ImGui::PopID();
    }

    if(o.type == cmp->point)
    {
        glPointSize(10);
        glUseProgram(o.program[0].ID);
        glBindVertexArray(cmp->line.ID);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    if(o.type == cmp->vector)
    {
        glPointSize(40);
        glBindVertexArray(cmp->line.ID);
        glUseProgram(o.program[0].ID);
        glDrawArrays(GL_LINES, 0, 2);
        glUseProgram(o.program[1].ID);
        glDrawArrays(GL_POINTS, 1, 1);
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

void tri(vec2 angle, vec3 vecs[3])
{
    vecs[0] = vec3(sin(-angle.y)*cos(angle.x), sin(-angle.y)*sin(angle.x), cos(-angle.y));
    vecs[1] = vec3(-sin(angle.x), cos(angle.x), 0);
    vecs[2] = cross(vecs[0], vecs[1]);
}

mat4 persp;
mat4 look;
vec3 center = vec3(0, 0, 0);
vec2 angle = vec2(0, 0);

int main(int, char**)
{
    cmp = new Compiler;
    tex = new Texture;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 460";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "tcc", NULL, NULL);
    if(!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

	GLenum err = glewInit();
	if (GLEW_OK != err)
		printf("Error: %s\n", glewGetErrorString(err));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", 18);
    ImFont * f2 = io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", 24);

    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glEnable(GL_DEPTH_TEST);
    glLineWidth(20);
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);

    tex->load("test.jpg");

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		{
			ImGui::Begin("Program");
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
                    cmp->frameSize = {800, 800};
                    cmp->compile(text);
                    msg = "OK";
                    persp = glm::perspective<float>(120, 1, 0.1, 10);
                    vec3 vecs[3];
                    tri(angle, vecs);
                    look = glm::lookAt<float>(center, center + vecs[0], vecs[2]);
                    mat4 camera = persp * look;
                    glBindBuffer(GL_UNIFORM_BUFFER, cmp->block.ID);
                    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &camera);
                    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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

            glBindFramebuffer(GL_FRAMEBUFFER, cmp->uvFrame.ID);
            glViewport(0, 0, cmp->frameSize.width, cmp->frameSize.height);
            glClearColor(0, 0, -1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindFramebuffer(GL_FRAMEBUFFER, cmp->frame.ID);
            glViewport(0, 0, cmp->frameSize.width, cmp->frameSize.height);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

                    glUseProgram(o.program[0].ID);

                    if(col[0] != o.col[0] ||
                    col[1] != o.col[1] ||
                    col[2] != o.col[2] ||
                    col[3] != o.col[3])
                    {
                        glUniform4f(0, o.col[0], o.col[1], o.col[2], o.col[3]);

                        if(o.type == cmp->vector)
                        {
                            glUseProgram(o.program[1].ID);
                            glUniform4f(0, o.col[0], o.col[1], o.col[2], o.col[3]);
                        }
                    }

                    glUseProgram(o.program[0].ID);
                    draw(o);
                }
            }

            ImGui::End();
    
            ImGui::Begin("Output");
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("btn", ImVec2(cmp->frameSize.width, cmp->frameSize.height), ImGuiButtonFlags_MouseButtonLeft);
            bool held = ImGui::IsItemActive();
            bool hover = ImGui::IsItemHovered();

            if(hover)
            {
                int x = io.MousePos.x - pos.x;
                int y = io.MousePos.y - pos.y;

                if(x >= 0)
                if(y >= 0)
                if(x < (int)cmp->frameSize.width)
                if(y < (int)cmp->frameSize.height)
                {
                    float data[3] = {0, 0, -1};
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, cmp->uvFrame.ID);
                    glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, data);
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                }
            }

            if(held)
            {
                ImGuiIO& io = ImGui::GetIO();
                vec2 delta = vec2(io.MouseDelta.x, io.MouseDelta.y);
                angle += delta/512.0f*6.0f;
                vec3 vecs[3];
                if(angle[0] < 0) angle[0] = Parser::CPI*2;
                if(angle[0] > Parser::CPI*2) angle[0] = 0;
                if(angle[1] < 0) angle[1] = 0;
                if(angle[1] > Parser::CPI) angle[1] = Parser::CPI;
                tri(angle, vecs);
                float w = ImGui::IsKeyDown('W') ? 1 : 0;
                float s = ImGui::IsKeyDown('S') ? 1 : 0;
                float a = ImGui::IsKeyDown('A') ? 1 : 0;
                float d = ImGui::IsKeyDown('D') ? 1 : 0;
                
                center += io.DeltaTime*((w-s)*vecs[0]+(a-d)*vecs[1]);
                look = glm::lookAt<float>(center, center + vecs[0], vecs[2]);
                mat4 camera = persp * look;
                glBindBuffer(GL_UNIFORM_BUFFER, cmp->block.ID);
                glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &camera);
                glBindBuffer(GL_UNIFORM_BUFFER, 0);
            }

            ImGui::SetCursorScreenPos(pos);
            ImGui::Image((void*)(intptr_t)cmp->frame.textures[0]->ID, ImVec2(cmp->frameSize.width, cmp->frameSize.height));
            ImGui::End();
        }

        _t = _t2;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
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
    delete tex;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}