#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <parser.hpp>
#include <stack>
#include <vector>

#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace tcc;
using std::vector;
using std::stack;
using std::unique_ptr;
using std::make_unique;

struct Expr
{
    char type{};
    //union
    //{
        Expr *child[2]{};
        Table *name;
        double number;
    //};
};

struct Int
{
    char type{};
    Table *tag{};
    char wrap{};
    Expr *e[3]{};
};

struct Obj
{
    Table *type{};
    Table *name{};
    Expr *e[2]{};
    std::vector<Int*> ints;
};

struct Compiler : public Parser
{
    std::vector<unique_ptr<Expr>> expr;
    std::vector<unique_ptr<Int>> ints;
    std::vector<unique_ptr<Obj>> objs;
    stack<Expr*> es;
    stack<Int*> is;

    void actAdvance()
    {
        
    }

    void actInt(char type)
    {
        Expr *e[3]{};
        e[0] = es.top(); es.pop();
        e[1] = es.top(); es.pop();
        if(type == 'g')
        {
            e[2] = es.top();
            es.pop();
        }
        Int i = {type, tag, wrap, {e[0], e[1], e[2]}};
        ints.push_back(make_unique<Int>(i));
        is.push(ints.back().get());
    }

    void actDecl()
    {
        Obj obj;
        obj.type = objType;
        obj.name = objName;

        if(objType == param || objType == grid)
        {
            while(!is.empty())
            {
                obj.ints.push_back(is.top());
                is.pop();
            }
        }
        else if(objType == define)
        {
            obj.e[0] = es.top();
            es.pop();
        }
        else if(objType == curve || objType == surface || objType == function)
        {
            obj.e[0] = es.top();
            es.pop();
            if(objType != function)
            while(!is.empty())
            {
                obj.ints.push_back(is.top());
                is.pop();
            }
        }
        else if(objType == point || objType == vector)
        {
            obj.e[0] = es.top();
            es.pop();
            if(objType == vector)
            {
                obj.e[1] = es.top();
                es.pop();
            }
        }
    }

    void actBinary(char type)
    {
        Expr exp;
        exp.type = type;
        switch(type)
        {
            case '_':
                exp.child[0] = es.top();
                es.pop();
                exp.name = lexer.node;
                break;
            case '.':
                exp.child[0] = es.top();
                es.pop();
                exp.number = lexer.number;
                if((int)exp.number != exp.number || (int)exp.number < 1) throw std::string("Component must be a positive integer");
                break;
            case '+':
            case '-':
            case '*':
            case '/':
            case 'j':
            case 'A':
            case 'E':
            case '^':
                exp.child[0] = es.top();
                es.pop();
                exp.child[1] = es.top();
                es.pop();
                break;
            case ')':
                {
                    Expr *right = nullptr;
                    Expr *exp = es.top();
                    es.pop();
                    printf("TUPLESIZE = %d\n", tupleSize);
                    while(--tupleSize)
                    {
                        Expr e;
                        e.type = ',';
                        e.child[0] = exp;
                        e.child[1] = right;
                        expr.push_back(make_unique<Expr>(e));
                        right = expr.back().get();
                        exp = es.top();
                        es.pop();
                    }
                    es.push(exp);
                }
                return;
        }

        expr.push_back(make_unique<Expr>(exp));
        es.push(expr.back().get());
    }

    void actUnary(char type)
    {
        Expr exp;
        exp.type = type;
        switch(type)
        {
            case 'P':
            case 'M':
            case 'T':
            case 'D':
            case '\'':
                exp.child[0] = es.top();
                es.pop();
                break;
            case 'N':
                exp.number = lexer.number;
                break;
            case 'C':
            case 'V':
            case 'F':
                exp.name = lexer.node;
                break;
        }
        expr.push_back(make_unique<Expr>(exp));
        es.push(expr.back().get());
    }
};


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
                    if(cmp.argList[t->argsIndex].size() != 1) throw std::string("`f` must be a single variable function");

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