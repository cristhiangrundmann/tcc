#pragma once

namespace tcc
{

    enum class Style
    {
        COMMENT = 4, DECLARE, FUNCTION, CONSTANT, 
        VARIABLE, NUMBER, UNDEFINED, SYMBOL, ERROR, BRACKETS
    };

    struct StyleSpec
    {
        unsigned char r, g, b, f;
        //f : 0 0 0 0 0 F I B
        //F: background / foreground
        //I: italic / B: bold
    };

    const StyleSpec ST_SYMBOL        = {0, 0, 0, 0};
    const StyleSpec ST_COMMENT       = {0, 100, 0, 3};
    const StyleSpec ST_DECLARE       = {50, 50, 200, 1};
    const StyleSpec ST_FUNCTION      = {0, 0, 0, 2};
    const StyleSpec ST_CONSTANT      = {100, 120, 150, 2};
    const StyleSpec ST_VARIABLE      = {242, 150, 11, 2};
    const StyleSpec ST_NUMBER        = {100, 100, 255, 2};
    const StyleSpec ST_UNDEFINED     = {242, 150, 11, 2};
    const StyleSpec ST_ERROR         = {255, 0, 0, 1};
    const StyleSpec ST_BRACKETS      = {150, 255, 150, 1};

}