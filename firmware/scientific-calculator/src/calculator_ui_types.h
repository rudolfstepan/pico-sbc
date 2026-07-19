#ifndef CALCULATOR_UI_TYPES_H
#define CALCULATOR_UI_TYPES_H

#include <stdint.h>

typedef enum {
    ACT_INSERT,
    ACT_EVALUATE,
    ACT_CLEAR,
    ACT_DELETE,
    ACT_PAGE,
    ACT_ANGLE,
    ACT_PROG_DIGIT,
    ACT_PROG_BASE,
    ACT_PROG_BINARY,
    ACT_PROG_UNARY,
    ACT_GOTO_PROGRAMMER,
    ACT_FMT_WIDTH,
    ACT_FMT_ACTION,
    ACT_FMT_VIEW,
    ACT_FMT_GOTO_BASE,
    ACT_GOTO_GRAPH,
    ACT_GOTO_TOOLS,
    ACT_MEMORY,
    ACT_HISTORY,
    ACT_CURSOR,
    ACT_SYMBOL_STORE,
    ACT_SYMBOL_FUNCTION,
    ACT_SYMBOL_EDIT,
    ACT_SYMBOL_SAVE,
    ACT_FAVORITE,
    ACT_FAVORITE_SET,
    ACT_GRAPH,
    ACT_LOGIC
} calc_action_t;

typedef enum {
    PAGE_BASIC,
    PAGE_SCIENTIFIC,
    PAGE_PROGRAMMER,
    PAGE_FORMAT,
    PAGE_TOOLS,
    PAGE_SYMBOLS,
    PAGE_GRAPH,
    PAGE_LOGIC
} calc_page_t;

typedef enum {
    FORMAT_VIEW_CONVERSIONS,
    FORMAT_VIEW_BITS,
    FORMAT_VIEW_IEEE32,
    FORMAT_VIEW_IEEE64
} calculator_format_view_t;

typedef enum {
    STYLE_NUMBER,
    STYLE_FUNCTION,
    STYLE_COMMAND,
    STYLE_EQUALS
} key_style_t;

typedef struct {
    const char *label;
    const char *token;
    uint8_t col;
    uint8_t row;
    calc_action_t action;
    key_style_t style;
} calc_key_t;

#endif
