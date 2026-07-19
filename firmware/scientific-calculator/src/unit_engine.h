#ifndef UNIT_ENGINE_H
#define UNIT_ENGINE_H

#include <stddef.h>

typedef enum {
    UNIT_CATEGORY_LENGTH,
    UNIT_CATEGORY_AREA,
    UNIT_CATEGORY_VOLUME,
    UNIT_CATEGORY_MASS,
    UNIT_CATEGORY_TIME,
    UNIT_CATEGORY_TEMPERATURE,
    UNIT_CATEGORY_ANGLE,
    UNIT_CATEGORY_PRESSURE,
    UNIT_CATEGORY_ENERGY,
    UNIT_CATEGORY_POWER,
    UNIT_CATEGORY_COUNT
} unit_category_t;

typedef enum {
    UNIT_STATUS_OK,
    UNIT_STATUS_INVALID_UNIT,
    UNIT_STATUS_INCOMPATIBLE,
    UNIT_STATUS_DOMAIN,
    UNIT_STATUS_NONFINITE
} unit_status_t;

typedef struct {
    unit_category_t category;
    const char *name;
    const char *symbol;
    double scale;
    double offset;
} unit_definition_t;

typedef struct {
    const char *name;
    const char *symbol;
    double value;
    const char *unit;
    const char *source;
} physical_constant_t;

const char *unit_engine_category_name(unit_category_t category);
size_t unit_engine_unit_count(unit_category_t category);
const unit_definition_t *unit_engine_unit(unit_category_t category,
                                          size_t index);
unit_status_t unit_engine_convert(double value,
                                  const unit_definition_t *from,
                                  const unit_definition_t *to,
                                  double *result);
const char *unit_engine_status_text(unit_status_t status);

size_t unit_engine_constant_count(void);
const physical_constant_t *unit_engine_constant(size_t index);

#endif
