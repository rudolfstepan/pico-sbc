#include "unit_engine.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int close_enough(double actual, double expected, double tolerance) {
    return fabs(actual - expected) <= tolerance * fmax(1.0, fabs(expected));
}

int main(void) {
    CHECK(UNIT_CATEGORY_COUNT == 10);
    size_t total_units = 0;
    for (unit_category_t category = UNIT_CATEGORY_LENGTH;
         category < UNIT_CATEGORY_COUNT; ++category) {
        CHECK(unit_engine_unit_count(category) >= 4);
        total_units += unit_engine_unit_count(category);
        CHECK(strcmp(unit_engine_category_name(category), "INVALID") != 0);
    }
    CHECK(total_units == 68);

    for (unit_category_t category = UNIT_CATEGORY_LENGTH;
         category < UNIT_CATEGORY_COUNT; ++category) {
        const unit_definition_t *base = unit_engine_unit(category, 0);
        double original = category == UNIT_CATEGORY_TEMPERATURE
            ? 300.0 : 1.2345;
        size_t count = unit_engine_unit_count(category);
        for (size_t index = 0; index < count; ++index) {
            const unit_definition_t *target = unit_engine_unit(category,
                                                                index);
            double converted = 0.0;
            double round_trip = 0.0;
            CHECK(unit_engine_convert(original, base, target, &converted) ==
                  UNIT_STATUS_OK);
            CHECK(unit_engine_convert(converted, target, base, &round_trip) ==
                  UNIT_STATUS_OK);
            CHECK(close_enough(round_trip, original, 1e-11));
        }
    }

    const unit_definition_t *metre = unit_engine_unit(UNIT_CATEGORY_LENGTH, 2);
    const unit_definition_t *mile = unit_engine_unit(UNIT_CATEGORY_LENGTH, 7);
    const unit_definition_t *kilogram = unit_engine_unit(UNIT_CATEGORY_MASS, 2);
    double result = 0.0;
    CHECK(unit_engine_convert(1.0, mile, metre, &result) == UNIT_STATUS_OK);
    CHECK(close_enough(result, 1609.344, 1e-12));
    CHECK(unit_engine_convert(result, metre, mile, &result) == UNIT_STATUS_OK);
    CHECK(close_enough(result, 1.0, 1e-12));
    CHECK(unit_engine_convert(1.0, metre, kilogram, &result) ==
          UNIT_STATUS_INCOMPATIBLE);

    const unit_definition_t *kelvin =
        unit_engine_unit(UNIT_CATEGORY_TEMPERATURE, 0);
    const unit_definition_t *celsius =
        unit_engine_unit(UNIT_CATEGORY_TEMPERATURE, 1);
    const unit_definition_t *fahrenheit =
        unit_engine_unit(UNIT_CATEGORY_TEMPERATURE, 2);
    CHECK(unit_engine_convert(0.0, celsius, fahrenheit, &result) ==
          UNIT_STATUS_OK);
    CHECK(close_enough(result, 32.0, 1e-12));
    CHECK(unit_engine_convert(-40.0, fahrenheit, celsius, &result) ==
          UNIT_STATUS_OK);
    CHECK(close_enough(result, -40.0, 1e-12));
    CHECK(unit_engine_convert(-1.0, kelvin, celsius, &result) ==
          UNIT_STATUS_DOMAIN);
    CHECK(unit_engine_convert(INFINITY, kelvin, celsius, &result) ==
          UNIT_STATUS_NONFINITE);

    const unit_definition_t *degree = unit_engine_unit(UNIT_CATEGORY_ANGLE, 1);
    const unit_definition_t *radian = unit_engine_unit(UNIT_CATEGORY_ANGLE, 0);
    CHECK(unit_engine_convert(180.0, degree, radian, &result) ==
          UNIT_STATUS_OK);
    CHECK(close_enough(result, 3.14159265358979323846, 1e-12));

    CHECK(unit_engine_constant_count() >= 10);
    const physical_constant_t *light = unit_engine_constant(0);
    CHECK(light && light->value == 299792458.0);
    CHECK(strstr(light->source, "BIPM") != NULL);
    CHECK(unit_engine_constant(unit_engine_constant_count()) == NULL);

    puts("unit engine tests passed");
    return 0;
}
