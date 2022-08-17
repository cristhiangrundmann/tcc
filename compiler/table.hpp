#pragma once
#include <memory>
#include <string>

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
        FUNCTION,
        COMMENT
    };

    TokenType charToken(char c);

    int alphIndex(char c);

    std::string getTypeString(TokenType type);

    struct Table
    {
        Table *parent{};
        std::unique_ptr<Table> children[62];
        int argIndex = -1;
        int objIndex = -1;
        int length = 0;
        TokenType type = TokenType::UNDEFINED;
        char character{};

        Table *next(char c);
        Table *procString(const char *str, bool match);
        Table *initString(const char *str, TokenType type);
        std::string getString();
    };

}