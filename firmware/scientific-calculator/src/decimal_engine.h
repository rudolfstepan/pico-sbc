#ifndef DECIMAL_ENGINE_H
#define DECIMAL_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DECIMAL_ENGINE_MAX_DIGITS 80u
#define DECIMAL_ENGINE_TEXT_CAPACITY 96u
#define DECIMAL_ENGINE_PACKED_CAPACITY 44u

typedef enum {
    DECIMAL_STATUS_OK,
    DECIMAL_STATUS_UNSUPPORTED,
    DECIMAL_STATUS_SYNTAX,
    DECIMAL_STATUS_DIV_ZERO,
    DECIMAL_STATUS_PRECISION,
    DECIMAL_STATUS_RANGE
} decimal_status_t;

typedef struct {
    char text[DECIMAL_ENGINE_TEXT_CAPACITY];
    double approximation;
    bool exact;
} decimal_result_t;

decimal_status_t decimal_engine_evaluate(const char *expression,
                                         const char *ans_text,
                                         decimal_result_t *result,
                                         int *error_position);
bool decimal_engine_pack_text(const char *text, uint8_t *packed,
                              size_t packed_size);
bool decimal_engine_unpack_text(const uint8_t *packed, size_t packed_size,
                                char *text, size_t text_size);
const char *decimal_status_text(decimal_status_t status);

#endif
