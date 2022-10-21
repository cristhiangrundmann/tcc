#pragma once
#include <memory>
#include <string>

namespace tcc
{
    /*
        TokenType
    A token type is either an enum or a char
    */
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

    //converts char to children index, for the symble table trie
    int alphIndex(char c);

    //enum names to string
    std::string getTypeString(TokenType type);

    /*
        Table
    Symbol table of all identifiers, implemented as trie
    parent: the parent of the node, or nullptr for the root
    children: the 62 `a-zA-Z0-9' children
    argIndex: index in argList (if symbol is a non-primitive function)
    objIndex: index in object list(if sumbol is an object)
    type: the type of the identifier
    character: last char of identifier
    str: string of the full identifier

    next: gets the node by appending `c' to the current identifier
        c: character to append
    procString: gets the identifier in `str'
        str: pointer to start of identifier
        match: tells whether largest valid string or the full identifier is obtained
    initString: initializes an identifier
        str: pointer to start of identifier
        type: token type to initialize
    */
    struct Table
    {
        Table *parent{};
        std::unique_ptr<Table> children[62];
        int argIndex = -1;
        int objIndex = -1;
        int length = 0;
        TokenType type = TokenType::UNDEFINED;
        char character{};
        std::string str{};

        Table *next(char c);
        Table *procString(const char *str, bool match);
        Table *initString(const char *str, TokenType type);
    };

}