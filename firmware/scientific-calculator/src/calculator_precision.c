#include "calculator_precision.h"

#include <ctype.h>
#include <string.h>

static bool text_is(const char *text, const char *expected) {
    if (!text || !expected) return false;
    while (*text && *expected) {
        if (toupper((unsigned char)*text++) !=
            toupper((unsigned char)*expected++)) {
            return false;
        }
    }
    return !*text && !*expected;
}

unsigned int calculator_precision_digits(calculator_precision_t precision) {
    switch (precision) {
        case CALCULATOR_PRECISION_NORMAL: return 40u;
        case CALCULATOR_PRECISION_ULTRA: return 128u;
        case CALCULATOR_PRECISION_HIGH:
        default: return 80u;
    }
}

unsigned int calculator_precision_work_bits(calculator_precision_t precision) {
    /* Keep ample binary guard bits beyond each requested decimal precision. */
    switch (precision) {
        case CALCULATOR_PRECISION_NORMAL: return 192u;
        case CALCULATOR_PRECISION_ULTRA: return 512u;
        case CALCULATOR_PRECISION_HIGH:
        default: return 320u;
    }
}

const char *calculator_precision_name(calculator_precision_t precision) {
    switch (precision) {
        case CALCULATOR_PRECISION_NORMAL: return "NORMAL";
        case CALCULATOR_PRECISION_ULTRA: return "ULTRA";
        case CALCULATOR_PRECISION_HIGH:
        default: return "HIGH";
    }
}

const char *calculator_precision_label(calculator_precision_t precision) {
    switch (precision) {
        case CALCULATOR_PRECISION_NORMAL: return "P40";
        case CALCULATOR_PRECISION_ULTRA: return "P128";
        case CALCULATOR_PRECISION_HIGH:
        default: return "P80";
    }
}

bool calculator_precision_parse(const char *text,
                                calculator_precision_t *precision) {
    if (!precision) return false;
    if (text_is(text, "NORMAL") || text_is(text, "40") ||
        text_is(text, "P40")) {
        *precision = CALCULATOR_PRECISION_NORMAL;
        return true;
    }
    if (text_is(text, "HIGH") || text_is(text, "80") ||
        text_is(text, "P80")) {
        *precision = CALCULATOR_PRECISION_HIGH;
        return true;
    }
    if (text_is(text, "ULTRA") || text_is(text, "128") ||
        text_is(text, "P128")) {
        *precision = CALCULATOR_PRECISION_ULTRA;
        return true;
    }
    return false;
}
