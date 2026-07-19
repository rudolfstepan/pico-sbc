#ifndef CALCULATOR_STORAGE_H
#define CALCULATOR_STORAGE_H

#include "calculator_persistence.h"

typedef enum {
    CALCULATOR_STORAGE_LOADED,
    CALCULATOR_STORAGE_DEFAULTS,
    CALCULATOR_STORAGE_RECOVERED
} calculator_storage_load_status_t;

typedef enum {
    CALCULATOR_STORAGE_WRITTEN,
    CALCULATOR_STORAGE_UNCHANGED,
    CALCULATOR_STORAGE_ERROR
} calculator_storage_save_status_t;

calculator_storage_load_status_t calculator_storage_load(
    calculator_persisted_state_t *state);
calculator_storage_save_status_t calculator_storage_save(
    const calculator_persisted_state_t *state);

#endif
