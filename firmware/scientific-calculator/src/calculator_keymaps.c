#include "calculator_keymaps.h"

static const calc_key_t basic_keys[] = {
    {"SCI", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"DEG", "", 1, 0, ACT_ANGLE, STYLE_COMMAND},
    {"(", "(", 2, 0, ACT_INSERT, STYLE_FUNCTION},
    {")", ")", 3, 0, ACT_INSERT, STYLE_FUNCTION},
    {"DEL", "", 4, 0, ACT_DELETE, STYLE_COMMAND},
    {"AC", "", 5, 0, ACT_CLEAR, STYLE_COMMAND},

    {"7", "7", 0, 1, ACT_INSERT, STYLE_NUMBER},
    {"8", "8", 1, 1, ACT_INSERT, STYLE_NUMBER},
    {"9", "9", 2, 1, ACT_INSERT, STYLE_NUMBER},
    {"/", "/", 3, 1, ACT_INSERT, STYLE_FUNCTION},
    {"SQRT", "sqrt(", 4, 1, ACT_INSERT, STYLE_FUNCTION},
    {"X^2", "^2", 5, 1, ACT_INSERT, STYLE_FUNCTION},

    {"4", "4", 0, 2, ACT_INSERT, STYLE_NUMBER},
    {"5", "5", 1, 2, ACT_INSERT, STYLE_NUMBER},
    {"6", "6", 2, 2, ACT_INSERT, STYLE_NUMBER},
    {"*", "*", 3, 2, ACT_INSERT, STYLE_FUNCTION},
    {"1/X", "1/(", 4, 2, ACT_INSERT, STYLE_FUNCTION},
    {"X^Y", "^", 5, 2, ACT_INSERT, STYLE_FUNCTION},

    {"1", "1", 0, 3, ACT_INSERT, STYLE_NUMBER},
    {"2", "2", 1, 3, ACT_INSERT, STYLE_NUMBER},
    {"3", "3", 2, 3, ACT_INSERT, STYLE_NUMBER},
    {"-", "-", 3, 3, ACT_INSERT, STYLE_FUNCTION},
    {"PI", "pi", 4, 3, ACT_INSERT, STYLE_FUNCTION},
    {"%", "%", 5, 3, ACT_INSERT, STYLE_FUNCTION},

    {"0", "0", 0, 4, ACT_INSERT, STYLE_NUMBER},
    {".", ".", 1, 4, ACT_INSERT, STYLE_NUMBER},
    {"ANS", "ans", 2, 4, ACT_INSERT, STYLE_FUNCTION},
    {"+", "+", 3, 4, ACT_INSERT, STYLE_FUNCTION},
    {",", ",", 4, 4, ACT_INSERT, STYLE_FUNCTION},
    {"=", "", 5, 4, ACT_EVALUATE, STYLE_EQUALS},
};

static const calc_key_t scientific_keys[] = {
    {"PROG", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"DEG", "", 1, 0, ACT_ANGLE, STYLE_COMMAND},
    {"(", "(", 2, 0, ACT_INSERT, STYLE_FUNCTION},
    {")", ")", 3, 0, ACT_INSERT, STYLE_FUNCTION},
    {"DEL", "", 4, 0, ACT_DELETE, STYLE_COMMAND},
    {"AC", "", 5, 0, ACT_CLEAR, STYLE_COMMAND},

    {"SIN", "sin(", 0, 1, ACT_INSERT, STYLE_FUNCTION},
    {"COS", "cos(", 1, 1, ACT_INSERT, STYLE_FUNCTION},
    {"TAN", "tan(", 2, 1, ACT_INSERT, STYLE_FUNCTION},
    {"ASIN", "asin(", 3, 1, ACT_INSERT, STYLE_FUNCTION},
    {"ACOS", "acos(", 4, 1, ACT_INSERT, STYLE_FUNCTION},
    {"ATAN", "atan(", 5, 1, ACT_INSERT, STYLE_FUNCTION},

    {"LN", "ln(", 0, 2, ACT_INSERT, STYLE_FUNCTION},
    {"LOG", "log(", 1, 2, ACT_INSERT, STYLE_FUNCTION},
    {"SQRT", "sqrt(", 2, 2, ACT_INSERT, STYLE_FUNCTION},
    {"EXP", "exp(", 3, 2, ACT_INSERT, STYLE_FUNCTION},
    {"10^X", "10^(", 4, 2, ACT_INSERT, STYLE_FUNCTION},
    {"X^Y", "^", 5, 2, ACT_INSERT, STYLE_FUNCTION},

    {"SINH", "sinh(", 0, 3, ACT_INSERT, STYLE_FUNCTION},
    {"COSH", "cosh(", 1, 3, ACT_INSERT, STYLE_FUNCTION},
    {"TANH", "tanh(", 2, 3, ACT_INSERT, STYLE_FUNCTION},
    {"ABS", "abs(", 3, 3, ACT_INSERT, STYLE_FUNCTION},
    {"FLOOR", "floor(", 4, 3, ACT_INSERT, STYLE_FUNCTION},
    {",", ",", 5, 3, ACT_INSERT, STYLE_FUNCTION},

    {"PI", "pi", 0, 4, ACT_INSERT, STYLE_FUNCTION},
    {"E", "e", 1, 4, ACT_INSERT, STYLE_FUNCTION},
    {"FAC", "fac(", 2, 4, ACT_INSERT, STYLE_FUNCTION},
    {"NCR", "ncr(", 3, 4, ACT_INSERT, STYLE_FUNCTION},
    {"NPR", "npr(", 4, 4, ACT_INSERT, STYLE_FUNCTION},
    {"=", "", 5, 4, ACT_EVALUATE, STYLE_EQUALS},
};

static const calc_key_t programmer_keys[] = {
    {"FMT", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"DEC", "DEC", 1, 0, ACT_PROG_BASE, STYLE_COMMAND},
    {"HEX", "HEX", 2, 0, ACT_PROG_BASE, STYLE_COMMAND},
    {"BIN", "BIN", 3, 0, ACT_PROG_BASE, STYLE_COMMAND},
    {"DEL", "", 4, 0, ACT_DELETE, STYLE_COMMAND},
    {"AC", "", 5, 0, ACT_CLEAR, STYLE_COMMAND},

    {"A", "A", 0, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"B", "B", 1, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"C", "C", 2, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"D", "D", 3, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"E", "E", 4, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"F", "F", 5, 1, ACT_PROG_DIGIT, STYLE_NUMBER},

    {"7", "7", 0, 2, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"8", "8", 1, 2, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"9", "9", 2, 2, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"AND", "AND", 3, 2, ACT_PROG_BINARY, STYLE_FUNCTION},
    {"OR", "OR", 4, 2, ACT_PROG_BINARY, STYLE_FUNCTION},
    {"XOR", "XOR", 5, 2, ACT_PROG_BINARY, STYLE_FUNCTION},

    {"4", "4", 0, 3, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"5", "5", 1, 3, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"6", "6", 2, 3, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"<<", "SHL", 3, 3, ACT_PROG_UNARY, STYLE_FUNCTION},
    {">>", "SHR", 4, 3, ACT_PROG_UNARY, STYLE_FUNCTION},
    {"NOT", "NOT", 5, 3, ACT_PROG_UNARY, STYLE_FUNCTION},

    {"1", "1", 0, 4, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"2", "2", 1, 4, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"3", "3", 2, 4, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"0", "0", 3, 4, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"NEG", "NEG", 4, 4, ACT_PROG_UNARY, STYLE_FUNCTION},
    {"=", "", 5, 4, ACT_EVALUATE, STYLE_EQUALS},
};

static const calc_key_t format_keys[] = {
    {"TOOLS", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"PROG", "", 1, 0, ACT_GOTO_PROGRAMMER, STYLE_COMMAND},
    {"8BIT", "8", 2, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"16BIT", "16", 3, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"32BIT", "32", 4, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"64BIT", "64", 5, 0, ACT_FMT_WIDTH, STYLE_COMMAND},

    {"TCNEG", "NEG", 0, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"MASK", "MASK", 1, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"SEXT", "SEXT", 2, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q-", "Q-", 3, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q+", "Q+", 4, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q0", "Q0", 5, 1, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"ANS>F32", "A32", 0, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ANS>F64", "A64", 1, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"F32>ANS", "32A", 2, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"F64>ANS", "64A", 3, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ZERO", "ZERO", 4, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ONES", "ONES", 5, 2, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"MIN", "MIN", 0, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"MAX", "MAX", 1, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"+1", "INC", 2, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"-1", "DEC", 3, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q8", "Q8", 4, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q16", "Q16", 5, 3, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"Q24", "Q24", 0, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"DEC", "DEC", 1, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"HEX", "HEX", 2, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"BIN", "BIN", 3, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"<<", "SHL", 4, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {">>", "SHR", 5, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
};

static const calc_key_t tools_keys[] = {
    {"BASIC", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"GRAPH", "", 1, 0, ACT_GOTO_GRAPH, STYLE_COMMAND},
    {"M+", "M+", 2, 0, ACT_MEMORY, STYLE_FUNCTION},
    {"M-", "M-", 3, 0, ACT_MEMORY, STYLE_FUNCTION},
    {"MR", "MR", 4, 0, ACT_MEMORY, STYLE_FUNCTION},
    {"MC", "MC", 5, 0, ACT_MEMORY, STYLE_FUNCTION},

    {"PREV", "PREV", 0, 1, ACT_HISTORY, STYLE_FUNCTION},
    {"NEXT", "NEXT", 1, 1, ACT_HISTORY, STYLE_FUNCTION},
    {"USE", "USE", 2, 1, ACT_HISTORY, STYLE_FUNCTION},
    {"HCLR", "CLEAR", 3, 1, ACT_HISTORY, STYLE_FUNCTION},
    {"ANS", "ans", 4, 1, ACT_INSERT, STYLE_FUNCTION},
    {"X", "x", 5, 1, ACT_INSERT, STYLE_FUNCTION},

    {"<", "LEFT", 0, 2, ACT_CURSOR, STYLE_FUNCTION},
    {">", "RIGHT", 1, 2, ACT_CURSOR, STYLE_FUNCTION},
    {"HOME", "HOME", 2, 2, ACT_CURSOR, STYLE_FUNCTION},
    {"END", "END", 3, 2, ACT_CURSOR, STYLE_FUNCTION},
    {"DEL", "", 4, 2, ACT_DELETE, STYLE_COMMAND},
    {"AC", "", 5, 2, ACT_CLEAR, STYLE_COMMAND},

    {"SIN", "sin(", 0, 3, ACT_INSERT, STYLE_FUNCTION},
    {"COS", "cos(", 1, 3, ACT_INSERT, STYLE_FUNCTION},
    {"TAN", "tan(", 2, 3, ACT_INSERT, STYLE_FUNCTION},
    {"(", "(", 3, 3, ACT_INSERT, STYLE_FUNCTION},
    {")", ")", 4, 3, ACT_INSERT, STYLE_FUNCTION},
    {"SQRT", "sqrt(", 5, 3, ACT_INSERT, STYLE_FUNCTION},

    {"X^2", "^2", 0, 4, ACT_INSERT, STYLE_FUNCTION},
    {"X^3", "^3", 1, 4, ACT_INSERT, STYLE_FUNCTION},
    {"+", "+", 2, 4, ACT_INSERT, STYLE_FUNCTION},
    {"-", "-", 3, 4, ACT_INSERT, STYLE_FUNCTION},
    {"*", "*", 4, 4, ACT_INSERT, STYLE_FUNCTION},
    {"/", "/", 5, 4, ACT_INSERT, STYLE_FUNCTION},
};

static const calc_key_t graph_plot_keys[] = {
    {"TOOLS", "", 0, 4, ACT_GOTO_TOOLS, STYLE_COMMAND},
    {"F1", "F1", 1, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"F2", "F2", 2, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"F3", "F3", 3, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"ON/OFF", "TOGGLE", 4, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"MORE", "MENU", 5, 4, ACT_GRAPH, STYLE_EQUALS},
};

static const calc_key_t graph_menu_keys[] = {
    {"PLOT", "PLOT", 0, 4, ACT_GRAPH, STYLE_COMMAND},
    {"TABLE", "TABLE", 1, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"TRACE", "TRACE", 2, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"AUTO", "AUTO", 3, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"ANALYZE", "ANALYZE", 4, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"RANGE", "RANGE", 5, 4, ACT_GRAPH, STYLE_EQUALS},
};

static const calc_key_t graph_analysis_keys[] = {
    {"PLOT", "PLOT", 0, 4, ACT_GRAPH, STYLE_COMMAND},
    {"ROOT", "ROOT", 1, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"XING", "XING", 2, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"DERIV", "DERIV", 3, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"INTEGR", "INTEGR", 4, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"MORE", "ANMORE", 5, 4, ACT_GRAPH, STYLE_EQUALS},
};

static const calc_key_t graph_analysis_more_keys[] = {
    {"BACK", "ANBACK", 0, 4, ACT_GRAPH, STYLE_COMMAND},
    {"EXTREM", "EXTREM", 1, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"A=ANS", "A=ANS", 2, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"B=ANS", "B=ANS", 3, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"VIEW", "VIEWINT", 4, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"TOL", "TOL", 5, 4, ACT_GRAPH, STYLE_EQUALS},
};

static const calc_key_t graph_table_keys[] = {
    {"PLOT", "PLOT", 0, 4, ACT_GRAPH, STYLE_COMMAND},
    {"X-", "TABLE-", 1, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"X+", "TABLE+", 2, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"STEP-", "STEP-", 3, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"STEP+", "STEP+", 4, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"AUTO", "AUTO", 5, 4, ACT_GRAPH, STYLE_EQUALS},
};

static const calc_key_t graph_range_keys[] = {
    {"PLOT", "PLOT", 0, 4, ACT_GRAPH, STYLE_COMMAND},
    {"XW-", "XSPAN-", 1, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"XW+", "XSPAN+", 2, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"YW-", "YSPAN-", 3, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"YW+", "YSPAN+", 4, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"RESET", "RESET", 5, 4, ACT_GRAPH, STYLE_EQUALS},
};

const calc_key_t *calculator_graph_keymap(graph_view_t view, size_t *count) {
    switch (view) {
        case GRAPH_VIEW_MENU:
            *count = sizeof graph_menu_keys / sizeof graph_menu_keys[0];
            return graph_menu_keys;
        case GRAPH_VIEW_TABLE:
            *count = sizeof graph_table_keys / sizeof graph_table_keys[0];
            return graph_table_keys;
        case GRAPH_VIEW_ANALYSIS:
            *count = sizeof graph_analysis_keys / sizeof graph_analysis_keys[0];
            return graph_analysis_keys;
        case GRAPH_VIEW_ANALYSIS_MORE:
            *count = sizeof graph_analysis_more_keys /
                     sizeof graph_analysis_more_keys[0];
            return graph_analysis_more_keys;
        case GRAPH_VIEW_RANGE:
            *count = sizeof graph_range_keys / sizeof graph_range_keys[0];
            return graph_range_keys;
        case GRAPH_VIEW_PLOT:
        default:
            *count = sizeof graph_plot_keys / sizeof graph_plot_keys[0];
            return graph_plot_keys;
    }
}

const calc_key_t *calculator_keymap(calc_page_t page, size_t *count) {
    switch (page) {
        case PAGE_SCIENTIFIC:
            *count = sizeof scientific_keys / sizeof scientific_keys[0];
            return scientific_keys;
        case PAGE_PROGRAMMER:
            *count = sizeof programmer_keys / sizeof programmer_keys[0];
            return programmer_keys;
        case PAGE_FORMAT:
            *count = sizeof format_keys / sizeof format_keys[0];
            return format_keys;
        case PAGE_TOOLS:
            *count = sizeof tools_keys / sizeof tools_keys[0];
            return tools_keys;
        case PAGE_GRAPH:
            return calculator_graph_keymap(GRAPH_VIEW_PLOT, count);
        case PAGE_BASIC:
        default:
            *count = sizeof basic_keys / sizeof basic_keys[0];
            return basic_keys;
    }
}
