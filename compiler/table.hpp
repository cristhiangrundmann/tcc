#pragma once
#include <memory>

namespace tcc
{
    enum class TokenType : int
    {
        EOI = 256,
        UNDEFINED,
        DECLARE,
        CONSTANT,
        VARIABLE,
        NUMBER,
        FUNCTION,
        COMMENT
    };

    TokenType charToken(char c);

    int alphIndex(char c);

    const char *getTypeString(TokenType type);

    struct Table
    {
        Table *parent = nullptr;
        std::unique_ptr<Table> children[62];
        int argsIndex = -1;
        int length = 0;
        TokenType type = TokenType::UNDEFINED;
        char character = 0;

        Table *next(char c);
        Table *procString(const char *str, bool match);
        Table *initString(const char *str, TokenType type);
    };

}