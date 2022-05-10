#pragma once
#include <memory>

namespace tcc
{
    enum class TokenType
    {
        EOI = 256,
        UNDEFINED,
        DECLARE,
        CONSTANT,
        VARIABLE,
        NUMBER,
        FUNCTION
    };

    TokenType charToken(char c);

    int alphIndex(char c);

    const char *getTypeString(TokenType type);

    struct Table
    {
        Table *parent = nullptr;
        std::unique_ptr<Table> children[62];
        int argsIndex = -1;
        int objIndex = -1;
        int length = 0;
        TokenType type = TokenType::UNDEFINED;
        char character = 0;

        enum class Mode
        {
            MATCH, INSERT, MAX
        };

        Table *next(char c, bool expand);
        Table *procString(const char *str, Mode mode);
        Table *initString(const char *str, TokenType type);
    };

}