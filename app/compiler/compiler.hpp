#pragma once

#include "parser.hpp"
#include <glm/glm.hpp>
#include <sstream>
#include <imgui.h>

namespace tcc
{
    /*
        SymbExpr
    Node for a "symbolic" expression, that is,
    an expression in the input string `as-is'
    type: the type/operation of the node
    sub: the children/operands of the node
    name: variable/function/constant name (if any)
    number: the number as float (if any)
    */
    struct SymbExpr
    {
        Parser::ExprType type{};
        std::vector<SymbExpr*> sub{};
        Table *name{};
        float number{};
    };

    /*
        CompExpr
    Node for a "computable" expression, that is,
    an expression good for computing.
    Generated from a "symbolic" expression, replacing function
    definitions, computing derivatives and decompressing tuple operations
    Same as SymbExpr, except:
    -Type numeration is smaller
    -nTuple: 1 if a number, or the number of elements in the tuple
    */
    struct CompExpr
    {
        enum class ExprType
        {
            PLUS, MINUS, TIMES, DIVIDE,
            UPLUS, UMINUS, UDIVIDE,
            APP, FUNCTION,
            POW, COMPONENT,
            CONSTANT, NUMBER, VARIABLE, TUPLE
        };
        ExprType type{};
        std::vector<CompExpr*> sub{};
        Table *name{};
        float number{};
        int nTuple{};
    };

    /*
        Interval
    All information for an interval
    type: kind of interval
    sub: expressions given. [A, B, C]
    compSub: computable expressions from the given ones
    number: current parameter/grid value (if any)
    min, max: float values for limits
    offset: byte offset for the Shader Uniform Block
    animate: tells whether to animate parameter (if any)
    */
    struct Interval
    {
        Parser::ExprType type{};
        Table *tag{};
        SymbExpr *sub[3]{};
        CompExpr *compSub[3]{};
        float number{};
        float min{}, max{};
        int offset{};
        bool animate{};
    };

    typedef unsigned int uint;

    struct Size
    {
        uint width{};
        uint height{};
    };

    //Holds an opengl texture
    struct Texture
    {
        uint ID{};
        void create(Size size, uint base, uint format, uint type);
        void load(const char *file);
        ~Texture();
    };

    //Holds an opengl framebuffer
    struct Framebuffer
    {
        uint ID{};
        std::vector<Texture*> textures;
        ~Framebuffer();
    };

    //Holds an opengl shader
    struct Shader
    {
        uint ID{};
        void compile(uint type, const char *source);
        ~Shader();
    };

    //Holds an opengl program
    struct Program
    {
        uint ID{};
        std::vector<Shader*> shaders;
        void link();
        ~Program();
    };

    //Holds an opengl buffer
    struct Buffer
    {
        uint ID{};
        ~Buffer();
    };

    //Holds an opengl array
    struct Array
    {
        uint ID{};
        std::vector<Buffer*> buffers;
        void create1DGrid(uint nx, Interval &i);
        void create2DGrid(uint nx, uint ny, Interval &i, Interval &j);
        ~Array();
    };

    typedef float Color[4];

    /*
        Obj
    All information in a declared object
    type: type of the object
    name: name of the object
    sub: given expression(s) that define the object
    compSub: computable expression from the given ones
    intervals: all the intervals attached to the object
    grids: list of (index of a grid object that this object depends on)
    nTuple: number of tuple elements of object
    program: opengl program(s) compiled for the object
    array: opengl array build for object
    frame: opengl framebuffer for geodesic tracing (of surfaces)
    col: color of the object

    SURFACES ONLY:
        texture: opengl texture selected
        changed: tells when to redo the geodesic tracing
        center: camera origin for geodesic tracing
        X, Y: camera orientation for geodesic tracing
        zoom: length of X and Y
    */
    struct Obj
    {
        Table *type{};
        Table *name{};
        SymbExpr *sub[2]{};
        CompExpr *compSub[12]{};
        std::vector<Interval> intervals;
        std::vector<int> grids;
        int nTuple{};

        Program program[4]{};
        Array array{};
        Framebuffer frame{};
        Texture *texture{};

        bool changed = true;

        //GEO
        glm::vec2 center = glm::vec2(0.5985, 0.2344);
        glm::vec2 X = glm::vec2(0, 1);
        glm::vec2 Y = glm::vec2(1, 0);
        float zoom = 1;

        Color col{};
    };

    /*
        Subts
    Used to perform substitutions for calculating an expression
    or applying functions
    */
    struct Subst
    {
        Table *var{};
        CompExpr *exp{};
        float number{};
    };

    /*
        Compiler
    Implements the semantic actions for the parser
    symbExprs: holds all symbolic expressions
    compExprs: holds all computable expressions
    expStack: expression stack, used to build expression trees
    intStack: interval stack, used to store current intervals
    objects: list of the declared objects
    frameSize: width and height of 3d view
    geoSize: width and height of geodesic tracing
    compiled: tells if it has been compiled yet
    OPENGL SPECIFICS:
    block: uniform buffer block, used for colors, parameters, grids, and camera
    blockSize: size of the block
    frameMS: multisample framebuffer for 3d view
    frame: final framebuffer for 3d view
    quad: square array for geodesic tracing
    line: line array for points and vectors
    various shaders: default or global shaders

    actInt: handle intervals
    actOp: handle expressions
    actDecl: handle object declarations

    newExpr's: create nodes for expressions
    op's: assemble expression nodes

    COMPUTE FUNCTIONS:
        _comp: compute tuple element access
        compute: main function to convert SymbExpr's to CompExpr's
        substitute: applies function definition (non-primitive functions)
        derivative: compute derivatives (symbolically)
    calculate: compute float value of expression (if possible)
    dependencies: find all grid dependencies

    COMPILATION FUNCTIONS:
        compile(const char *source): main/driver function, generates everything
            source: the input string
        compile: generates GLSL source code for evaluating expressions
        header: generates GLSL header
        compileFunction: generates code for GLSL function definition
        declareFunction: generates GLSL function declaration
    */
    struct Compiler : public Parser
    {
        std::vector<std::unique_ptr<SymbExpr>> symbExprs;
        std::vector<std::unique_ptr<CompExpr>> compExprs;
        std::vector<SymbExpr*> expStack;
        std::vector<Interval> intStack;
        std::vector<Obj> objects;
        Size frameSize = {512, 512};
        Size geoSize = {512, 512};

        Buffer block{};
        uint blockSize{};
        Framebuffer frameMS{};
        Framebuffer frame{};
        Array quad{};
        Array line{};
        Shader defaultVert{};
        Shader pointFrag{};
        Shader arrowFrag{};
        Shader lineFrag{};
        Shader texFrag{};
        Shader uvFrag{};
        Shader lineGeom{};
        Shader pointGeom{};

        bool compiled = false;

        void actInt(ExprType type);
        void actOp(ExprType type);
        void actDecl();
        
        SymbExpr *newExpr(SymbExpr &e);
        CompExpr *newExpr(CompExpr &e);
        SymbExpr op(Parser::ExprType type, SymbExpr *a = nullptr, SymbExpr *b = nullptr, float number = 0, Table *name = nullptr);
        CompExpr *op(CompExpr::ExprType type, CompExpr *a = nullptr, CompExpr *b = nullptr, float number = 0, Table *name = nullptr, int nTuple = 1);
        CompExpr *_comp(CompExpr *e, unsigned int index, std::vector<Subst> &subs);
        CompExpr *compute(SymbExpr *e, std::vector<Subst> &subs);
        CompExpr *substitute(CompExpr *e, std::vector<Subst> &subs);
        CompExpr *derivative(CompExpr *e, Table *var);
        
        float calculate(CompExpr *e, std::vector<Subst> &subs);
        void dependencies(CompExpr *e, std::vector<int> &grids, bool allow = false);

        void compile(CompExpr *e, std::stringstream &str, int &v);
        void compile(const char *source);
        void header(std::stringstream &str);
        void compileFunction(CompExpr *exp, int argIndex, std::stringstream &str, std::string name);
        void declareFunction(int N, int argIndex, std::stringstream &str, std::string name, bool declareOnly = false);
    };

    /*
        Highlight
    Syntax Highlighting for text editor
    Implements lexer's semantic action actAdvance
    palette: points to palette string
    buf_palette: points to color string
    colosize: main function
    */
    struct Highlight : public Parser
    {
        static ImU32 palette[];
        char *buf_palette = nullptr;
        void actAdvance();
        void colorize(const char *source);
    };

}