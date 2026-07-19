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
    {"BITS", "BITS", 1, 0, ACT_FMT_VIEW, STYLE_COMMAND},
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

static const calc_key_t format_bits_keys[] = {
    {"TOOLS", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"CONV", "CONV", 1, 0, ACT_FMT_VIEW, STYLE_COMMAND},
    {"8BIT", "8", 2, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"16BIT", "16", 3, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"32BIT", "32", 4, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"64BIT", "64", 5, 0, ACT_FMT_WIDTH, STYLE_COMMAND},

    {"UNSIGNED", "SIGN", 0, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ROL", "ROL", 1, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ROR", "ROR", 2, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"SWAP", "SWAP", 3, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"IEEE32", "IEEE32", 4, 1, ACT_FMT_VIEW, STYLE_FUNCTION},
    {"IEEE64", "IEEE64", 5, 1, ACT_FMT_VIEW, STYLE_FUNCTION},

    {"BIT-8", "BIT-8", 0, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"BIT-1", "BIT-1", 1, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"BIT+1", "BIT+1", 2, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"BIT+8", "BIT+8", 3, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"SET", "BSET", 4, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"CLR", "BCLR", 5, 2, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"TOGGLE", "BTOGGLE", 0, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ZERO", "ZERO", 1, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ONES", "ONES", 2, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"MIN", "MIN", 3, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"MAX", "MAX", 4, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"NEG", "NEG", 5, 3, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"+1", "INC", 0, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"-1", "DEC", 1, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"<<", "SHL", 2, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {">>", "SHR", 3, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"C/V CLR", "FLAGS", 4, 4, ACT_FMT_ACTION, STYLE_COMMAND},
    {"PROG", "", 5, 4, ACT_GOTO_PROGRAMMER, STYLE_EQUALS},
};

static const calc_key_t format_ieee_keys[] = {
    {"TOOLS", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"CONV", "CONV", 1, 0, ACT_FMT_VIEW, STYLE_COMMAND},
    {"BITS", "BITS", 2, 0, ACT_FMT_VIEW, STYLE_COMMAND},
    {"F32", "IEEE32", 3, 0, ACT_FMT_VIEW, STYLE_COMMAND},
    {"F64", "IEEE64", 4, 0, ACT_FMT_VIEW, STYLE_COMMAND},
    {"PROG", "", 5, 0, ACT_GOTO_PROGRAMMER, STYLE_COMMAND},

    {"ANS>F32", "A32", 0, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ANS>F64", "A64", 1, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"F32>ANS", "32A", 2, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"F64>ANS", "64A", 3, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ZERO", "ZERO", 4, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ONES", "ONES", 5, 1, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"8BIT", "8", 0, 2, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"16BIT", "16", 1, 2, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"32BIT", "32", 2, 2, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"64BIT", "64", 3, 2, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"SWAP", "SWAP", 4, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"UNSIGNED", "SIGN", 5, 2, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"ROL", "ROL", 0, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ROR", "ROR", 1, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"<<", "SHL", 2, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {">>", "SHR", 3, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"+1", "INC", 4, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"-1", "DEC", 5, 3, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"DEC", "DEC", 0, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"HEX", "HEX", 1, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"BIN", "BIN", 2, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"C/V CLR", "FLAGS", 3, 4, ACT_FMT_ACTION, STYLE_COMMAND},
    {"MIN", "MIN", 4, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"MAX", "MAX", 5, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
};

static const calc_key_t tools_keys[] = {
    {"SYMBOLS", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
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

static const calc_key_t symbol_keys[] = {
    {"LOGIC", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"A", "A", 1, 0, ACT_INSERT, STYLE_NUMBER},
    {"B", "B", 2, 0, ACT_INSERT, STYLE_NUMBER},
    {"C", "C", 3, 0, ACT_INSERT, STYLE_NUMBER},
    {"D", "D", 4, 0, ACT_INSERT, STYLE_NUMBER},
    {"E", "E", 5, 0, ACT_INSERT, STYLE_NUMBER},

    {"F", "F", 0, 1, ACT_INSERT, STYLE_NUMBER},
    {"A=ANS", "A", 1, 1, ACT_SYMBOL_STORE, STYLE_FUNCTION},
    {"B=ANS", "B", 2, 1, ACT_SYMBOL_STORE, STYLE_FUNCTION},
    {"C=ANS", "C", 3, 1, ACT_SYMBOL_STORE, STYLE_FUNCTION},
    {"D=ANS", "D", 4, 1, ACT_SYMBOL_STORE, STYLE_FUNCTION},
    {"E=ANS", "E", 5, 1, ACT_SYMBOL_STORE, STYLE_FUNCTION},

    {"F=ANS", "F", 0, 2, ACT_SYMBOL_STORE, STYLE_FUNCTION},
    {"F1", "0", 1, 2, ACT_SYMBOL_FUNCTION, STYLE_FUNCTION},
    {"F2", "1", 2, 2, ACT_SYMBOL_FUNCTION, STYLE_FUNCTION},
    {"F3", "2", 3, 2, ACT_SYMBOL_FUNCTION, STYLE_FUNCTION},
    {"EDIT", "", 4, 2, ACT_SYMBOL_EDIT, STYLE_COMMAND},
    {"SAVE", "", 5, 2, ACT_SYMBOL_SAVE, STYLE_EQUALS},

    {"FAV1", "0", 0, 3, ACT_FAVORITE, STYLE_FUNCTION},
    {"FAV2", "1", 1, 3, ACT_FAVORITE, STYLE_FUNCTION},
    {"FAV3", "2", 2, 3, ACT_FAVORITE, STYLE_FUNCTION},
    {"FAV4", "3", 3, 3, ACT_FAVORITE, STYLE_FUNCTION},
    {"FAV5", "4", 4, 3, ACT_FAVORITE, STYLE_FUNCTION},
    {"FAV6", "5", 5, 3, ACT_FAVORITE, STYLE_FUNCTION},

    {"SET1", "0", 0, 4, ACT_FAVORITE_SET, STYLE_COMMAND},
    {"SET2", "1", 1, 4, ACT_FAVORITE_SET, STYLE_COMMAND},
    {"SET3", "2", 2, 4, ACT_FAVORITE_SET, STYLE_COMMAND},
    {"SET4", "3", 3, 4, ACT_FAVORITE_SET, STYLE_COMMAND},
    {"SET5", "4", 4, 4, ACT_FAVORITE_SET, STYLE_COMMAND},
    {"SET6", "5", 5, 4, ACT_FAVORITE_SET, STYLE_COMMAND},
};

static const calc_key_t logic_keys[] = {
    {"UNITS", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"CHECK", "CHECK", 1, 0, ACT_LOGIC, STYLE_COMMAND},
    {"TABLE", "TABLE", 2, 0, ACT_LOGIC, STYLE_COMMAND},
    {"DNF", "DNF", 3, 0, ACT_LOGIC, STYLE_COMMAND},
    {"KNF", "KNF", 4, 0, ACT_LOGIC, STYLE_COMMAND},
    {"GATES", "GATES", 5, 0, ACT_LOGIC, STYLE_EQUALS},

    {"A", "A", 0, 1, ACT_LOGIC, STYLE_NUMBER},
    {"B", "B", 1, 1, ACT_LOGIC, STYLE_NUMBER},
    {"C", "C", 2, 1, ACT_LOGIC, STYLE_NUMBER},
    {"D", "D", 3, 1, ACT_LOGIC, STYLE_NUMBER},
    {"E", "E", 4, 1, ACT_LOGIC, STYLE_NUMBER},
    {"F", "F", 5, 1, ACT_LOGIC, STYLE_NUMBER},

    {"NOT", "!", 0, 2, ACT_LOGIC, STYLE_FUNCTION},
    {"AND", "&", 1, 2, ACT_LOGIC, STYLE_FUNCTION},
    {"OR", "|", 2, 2, ACT_LOGIC, STYLE_FUNCTION},
    {"XOR", "^", 3, 2, ACT_LOGIC, STYLE_FUNCTION},
    {"NAND", " NAND ", 4, 2, ACT_LOGIC, STYLE_FUNCTION},
    {"NOR", " NOR ", 5, 2, ACT_LOGIC, STYLE_FUNCTION},

    {"XNOR", " XNOR ", 0, 3, ACT_LOGIC, STYLE_FUNCTION},
    {"(", "(", 1, 3, ACT_LOGIC, STYLE_FUNCTION},
    {")", ")", 2, 3, ACT_LOGIC, STYLE_FUNCTION},
    {"0", "0", 3, 3, ACT_LOGIC, STYLE_NUMBER},
    {"1", "1", 4, 3, ACT_LOGIC, STYLE_NUMBER},
    {"DEL", "DEL", 5, 3, ACT_LOGIC, STYLE_COMMAND},

    {"AC", "AC", 0, 4, ACT_LOGIC, STYLE_COMMAND},
    {"<", "LEFT", 1, 4, ACT_LOGIC, STYLE_FUNCTION},
    {">", "RIGHT", 2, 4, ACT_LOGIC, STYLE_FUNCTION},
    {"UP", "UP", 3, 4, ACT_LOGIC, STYLE_FUNCTION},
    {"DOWN", "DOWN", 4, 4, ACT_LOGIC, STYLE_FUNCTION},
    {"USE", "USE", 5, 4, ACT_LOGIC, STYLE_EQUALS},
};

static const calc_key_t unit_keys[] = {
    {"COMPLEX", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"<CAT", "CAT-", 1, 0, ACT_UNITS, STYLE_COMMAND},
    {"CAT>", "CAT+", 2, 0, ACT_UNITS, STYLE_COMMAND},
    {"<FROM", "FROM-", 3, 0, ACT_UNITS, STYLE_COMMAND},
    {"FROM>", "FROM+", 4, 0, ACT_UNITS, STYLE_COMMAND},
    {"SWAP", "SWAP", 5, 0, ACT_UNITS, STYLE_EQUALS},

    {"LEN", "LENGTH", 0, 1, ACT_UNITS, STYLE_NUMBER},
    {"AREA", "AREA", 1, 1, ACT_UNITS, STYLE_NUMBER},
    {"VOL", "VOLUME", 2, 1, ACT_UNITS, STYLE_NUMBER},
    {"MASS", "MASS", 3, 1, ACT_UNITS, STYLE_NUMBER},
    {"TIME", "TIME", 4, 1, ACT_UNITS, STYLE_NUMBER},
    {"TEMP", "TEMPERATURE", 5, 1, ACT_UNITS, STYLE_NUMBER},

    {"ANGLE", "ANGLE", 0, 2, ACT_UNITS, STYLE_FUNCTION},
    {"PRESS", "PRESSURE", 1, 2, ACT_UNITS, STYLE_FUNCTION},
    {"ENERGY", "ENERGY", 2, 2, ACT_UNITS, STYLE_FUNCTION},
    {"POWER", "POWER", 3, 2, ACT_UNITS, STYLE_FUNCTION},
    {"CONV", "CONV", 4, 2, ACT_UNITS, STYLE_COMMAND},
    {"CONST", "CONST", 5, 2, ACT_UNITS, STYLE_COMMAND},

    {"ANS>IN", "ANSIN", 0, 3, ACT_UNITS, STYLE_FUNCTION},
    {"<TO", "TO-", 1, 3, ACT_UNITS, STYLE_FUNCTION},
    {"TO>", "TO+", 2, 3, ACT_UNITS, STYLE_FUNCTION},
    {"CONVERT", "CONVERT", 3, 3, ACT_UNITS, STYLE_EQUALS},
    {"OUT>ANS", "OUTANS", 4, 3, ACT_UNITS, STYLE_FUNCTION},
    {"OUT>EDIT", "OUTEDIT", 5, 3, ACT_UNITS, STYLE_FUNCTION},

    {"C-", "C-", 0, 4, ACT_UNITS, STYLE_FUNCTION},
    {"C+", "C+", 1, 4, ACT_UNITS, STYLE_FUNCTION},
    {"C>ANS", "CANS", 2, 4, ACT_UNITS, STYLE_FUNCTION},
    {"C>EDIT", "CEDIT", 3, 4, ACT_UNITS, STYLE_FUNCTION},
    {"INFO", "INFO", 4, 4, ACT_UNITS, STYLE_COMMAND},
    {"RESET", "RESET", 5, 4, ACT_UNITS, STYLE_COMMAND},
};

static const calc_key_t complex_keys[] = {
    {"BASIC", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"CART", "VIEW", 1, 0, ACT_COMPLEX, STYLE_COMMAND},
    {"DEG", "", 2, 0, ACT_ANGLE, STYLE_COMMAND},
    {"DEL", "DEL", 3, 0, ACT_COMPLEX, STYLE_COMMAND},
    {"AC", "AC", 4, 0, ACT_COMPLEX, STYLE_COMMAND},
    {"=", "=", 5, 0, ACT_COMPLEX, STYLE_EQUALS},

    {"7", "7", 0, 1, ACT_COMPLEX, STYLE_NUMBER},
    {"8", "8", 1, 1, ACT_COMPLEX, STYLE_NUMBER},
    {"9", "9", 2, 1, ACT_COMPLEX, STYLE_NUMBER},
    {"/", "/", 3, 1, ACT_COMPLEX, STYLE_FUNCTION},
    {"(", "(", 4, 1, ACT_COMPLEX, STYLE_FUNCTION},
    {")", ")", 5, 1, ACT_COMPLEX, STYLE_FUNCTION},

    {"4", "4", 0, 2, ACT_COMPLEX, STYLE_NUMBER},
    {"5", "5", 1, 2, ACT_COMPLEX, STYLE_NUMBER},
    {"6", "6", 2, 2, ACT_COMPLEX, STYLE_NUMBER},
    {"*", "*", 3, 2, ACT_COMPLEX, STYLE_FUNCTION},
    {"I", "i", 4, 2, ACT_COMPLEX, STYLE_FUNCTION},
    {"CONJ", "conj(", 5, 2, ACT_COMPLEX, STYLE_FUNCTION},

    {"1", "1", 0, 3, ACT_COMPLEX, STYLE_NUMBER},
    {"2", "2", 1, 3, ACT_COMPLEX, STYLE_NUMBER},
    {"3", "3", 2, 3, ACT_COMPLEX, STYLE_NUMBER},
    {"-", "-", 3, 3, ACT_COMPLEX, STYLE_FUNCTION},
    {"ABS", "abs(", 4, 3, ACT_COMPLEX, STYLE_FUNCTION},
    {"ARG", "arg(", 5, 3, ACT_COMPLEX, STYLE_FUNCTION},

    {"0", "0", 0, 4, ACT_COMPLEX, STYLE_NUMBER},
    {".", ".", 1, 4, ACT_COMPLEX, STYLE_NUMBER},
    {"+", "+", 2, 4, ACT_COMPLEX, STYLE_FUNCTION},
    {",", ",", 3, 4, ACT_COMPLEX, STYLE_FUNCTION},
    {"POLAR", "polar(", 4, 4, ACT_COMPLEX, STYLE_FUNCTION},
    {"HIST", "HIST", 5, 4, ACT_COMPLEX, STYLE_COMMAND},
};

static const calc_key_t complex_history_keys[] = {
    {"BASIC", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"CART", "VIEW", 1, 0, ACT_COMPLEX, STYLE_COMMAND},
    {"DEG", "", 2, 0, ACT_ANGLE, STYLE_COMMAND},
    {"DEL", "DEL", 3, 0, ACT_COMPLEX, STYLE_COMMAND},
    {"AC", "AC", 4, 0, ACT_COMPLEX, STYLE_COMMAND},
    {"=", "=", 5, 0, ACT_COMPLEX, STYLE_EQUALS},

    {"7", "7", 0, 1, ACT_COMPLEX, STYLE_NUMBER},
    {"8", "8", 1, 1, ACT_COMPLEX, STYLE_NUMBER},
    {"9", "9", 2, 1, ACT_COMPLEX, STYLE_NUMBER},
    {"/", "/", 3, 1, ACT_COMPLEX, STYLE_FUNCTION},
    {"(", "(", 4, 1, ACT_COMPLEX, STYLE_FUNCTION},
    {")", ")", 5, 1, ACT_COMPLEX, STYLE_FUNCTION},

    {"4", "4", 0, 2, ACT_COMPLEX, STYLE_NUMBER},
    {"5", "5", 1, 2, ACT_COMPLEX, STYLE_NUMBER},
    {"6", "6", 2, 2, ACT_COMPLEX, STYLE_NUMBER},
    {"*", "*", 3, 2, ACT_COMPLEX, STYLE_FUNCTION},
    {"I", "i", 4, 2, ACT_COMPLEX, STYLE_FUNCTION},
    {"CONJ", "conj(", 5, 2, ACT_COMPLEX, STYLE_FUNCTION},

    {"1", "1", 0, 3, ACT_COMPLEX, STYLE_NUMBER},
    {"2", "2", 1, 3, ACT_COMPLEX, STYLE_NUMBER},
    {"3", "3", 2, 3, ACT_COMPLEX, STYLE_NUMBER},
    {"-", "-", 3, 3, ACT_COMPLEX, STYLE_FUNCTION},
    {"ABS", "abs(", 4, 3, ACT_COMPLEX, STYLE_FUNCTION},
    {"ARG", "arg(", 5, 3, ACT_COMPLEX, STYLE_FUNCTION},

    {"BACK", "BACK", 0, 4, ACT_COMPLEX, STYLE_COMMAND},
    {"PREV", "PREV", 1, 4, ACT_COMPLEX, STYLE_FUNCTION},
    {"NEXT", "NEXT", 2, 4, ACT_COMPLEX, STYLE_FUNCTION},
    {"USE", "USE", 3, 4, ACT_COMPLEX, STYLE_EQUALS},
    {"HCLR", "HCLR", 4, 4, ACT_COMPLEX, STYLE_COMMAND},
    {"VIEW", "VIEW", 5, 4, ACT_COMPLEX, STYLE_COMMAND},
};

const calc_key_t *calculator_complex_keymap(bool history, size_t *count) {
    if (history) {
        *count = sizeof complex_history_keys / sizeof complex_history_keys[0];
        return complex_history_keys;
    }
    *count = sizeof complex_keys / sizeof complex_keys[0];
    return complex_keys;
}

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

const calc_key_t *calculator_format_keymap(calculator_format_view_t view,
                                           size_t *count) {
    if (view == FORMAT_VIEW_BITS) {
        *count = sizeof format_bits_keys / sizeof format_bits_keys[0];
        return format_bits_keys;
    }
    if (view == FORMAT_VIEW_IEEE32 || view == FORMAT_VIEW_IEEE64) {
        *count = sizeof format_ieee_keys / sizeof format_ieee_keys[0];
        return format_ieee_keys;
    }
    *count = sizeof format_keys / sizeof format_keys[0];
    return format_keys;
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
            return calculator_format_keymap(FORMAT_VIEW_CONVERSIONS, count);
        case PAGE_TOOLS:
            *count = sizeof tools_keys / sizeof tools_keys[0];
            return tools_keys;
        case PAGE_SYMBOLS:
            *count = sizeof symbol_keys / sizeof symbol_keys[0];
            return symbol_keys;
        case PAGE_GRAPH:
            return calculator_graph_keymap(GRAPH_VIEW_PLOT, count);
        case PAGE_LOGIC:
            *count = sizeof logic_keys / sizeof logic_keys[0];
            return logic_keys;
        case PAGE_UNITS:
            *count = sizeof unit_keys / sizeof unit_keys[0];
            return unit_keys;
        case PAGE_COMPLEX:
            return calculator_complex_keymap(false, count);
        case PAGE_BASIC:
        default:
            *count = sizeof basic_keys / sizeof basic_keys[0];
            return basic_keys;
    }
}
