#pragma once

#include "parser.hpp"
#include <sstream>

namespace tcc
{
    struct SymbExpr
    {
        Parser::ExprType type{};
        std::vector<SymbExpr*> sub{};
        Table *name{};
        float number{};
    };

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

    struct Interval
    {
        Parser::ExprType type{};
        Table *tag{};
        char wrap{};
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

    struct Texture
    {
        uint ID{};
        void create(Size size, uint base, uint type);
        ~Texture();
    };

    struct Framebuffer
    {
        uint ID{};
        std::vector<Texture*> textures;
        ~Framebuffer();
    };

    struct Shader
    {
        uint ID{};
        void compile(uint type, const char *source);
        ~Shader();
    };

    struct Program
    {
        uint ID{};
        std::vector<Shader*> shaders;
        void link();
        ~Program();
    };

    struct Buffer
    {
        uint ID{};
        ~Buffer();
    };

    struct Array
    {
        uint ID{};
        std::vector<Buffer*> buffers;
        void create1DGrid(uint nx, Interval &i);
        void create2DGrid(uint nx, uint ny, Interval &i, Interval &j);
        ~Array();
    };

    typedef float Color[4];

    struct Obj
    {
        Table *type{};
        Table *name{};
        SymbExpr *sub[2]{};
        CompExpr *compSub[2]{};
        std::vector<Interval> intervals;
        std::vector<int> grids;
        int nTuple{};

        Program program{};
        Program program2{};
        Array array{};
        Framebuffer frame{};

        Color col{};
    };

    struct Subst
    {
        Table *var{};
        CompExpr *exp{};
        float number{};
    };

    struct Compiler : public Parser
    {
        std::vector<std::unique_ptr<SymbExpr>> symbExprs;
        std::vector<std::unique_ptr<CompExpr>> compExprs;
        std::vector<SymbExpr*> expStack;
        std::vector<Interval> intStack;
        std::vector<Obj> objects;

        Buffer block{};
        uint blockSize{};
        Framebuffer frame{};
        Array quad{};
        Array line{};
        Shader defaultVert{};
        Shader defaultFrag{};

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
        void compile(CompExpr *e, std::stringstream &str, int &v);
        void compile(const char *source);
        void header(std::stringstream &str);
        void compileFunction(CompExpr *exp, int argIndex, std::stringstream &str, std::string name);
        void declareFunction(int N, int argIndex, std::stringstream &str, std::string name, bool declareOnly = false);
        float calculate(CompExpr *e, std::vector<Subst> &subs);
        void dependencies(CompExpr *e, std::vector<int> &grids, bool allow = false);
    };

};