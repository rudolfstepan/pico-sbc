#include "unit_engine.h"

#include <math.h>

#define PI_VALUE 3.14159265358979323846

static const char *const category_names[] = {
    "LENGTH", "AREA", "VOLUME", "MASS", "TIME", "TEMPERATURE",
    "ANGLE", "PRESSURE", "ENERGY", "POWER"
};

static const unit_definition_t units[] = {
    {UNIT_CATEGORY_LENGTH, "millimetre", "mm", 1e-3, 0.0},
    {UNIT_CATEGORY_LENGTH, "centimetre", "cm", 1e-2, 0.0},
    {UNIT_CATEGORY_LENGTH, "metre", "m", 1.0, 0.0},
    {UNIT_CATEGORY_LENGTH, "kilometre", "km", 1e3, 0.0},
    {UNIT_CATEGORY_LENGTH, "inch", "in", 0.0254, 0.0},
    {UNIT_CATEGORY_LENGTH, "foot", "ft", 0.3048, 0.0},
    {UNIT_CATEGORY_LENGTH, "yard", "yd", 0.9144, 0.0},
    {UNIT_CATEGORY_LENGTH, "mile", "mi", 1609.344, 0.0},
    {UNIT_CATEGORY_LENGTH, "nautical mile", "nmi", 1852.0, 0.0},

    {UNIT_CATEGORY_AREA, "square millimetre", "mm2", 1e-6, 0.0},
    {UNIT_CATEGORY_AREA, "square centimetre", "cm2", 1e-4, 0.0},
    {UNIT_CATEGORY_AREA, "square metre", "m2", 1.0, 0.0},
    {UNIT_CATEGORY_AREA, "hectare", "ha", 1e4, 0.0},
    {UNIT_CATEGORY_AREA, "square kilometre", "km2", 1e6, 0.0},
    {UNIT_CATEGORY_AREA, "square inch", "in2", 0.00064516, 0.0},
    {UNIT_CATEGORY_AREA, "square foot", "ft2", 0.09290304, 0.0},
    {UNIT_CATEGORY_AREA, "acre", "acre", 4046.8564224, 0.0},

    {UNIT_CATEGORY_VOLUME, "millilitre", "mL", 1e-6, 0.0},
    {UNIT_CATEGORY_VOLUME, "litre", "L", 1e-3, 0.0},
    {UNIT_CATEGORY_VOLUME, "cubic metre", "m3", 1.0, 0.0},
    {UNIT_CATEGORY_VOLUME, "cubic inch", "in3", 1.6387064e-5, 0.0},
    {UNIT_CATEGORY_VOLUME, "cubic foot", "ft3", 0.028316846592, 0.0},
    {UNIT_CATEGORY_VOLUME, "US gallon", "gal US", 0.003785411784, 0.0},
    {UNIT_CATEGORY_VOLUME, "imperial gallon", "gal UK", 0.00454609, 0.0},

    {UNIT_CATEGORY_MASS, "milligram", "mg", 1e-6, 0.0},
    {UNIT_CATEGORY_MASS, "gram", "g", 1e-3, 0.0},
    {UNIT_CATEGORY_MASS, "kilogram", "kg", 1.0, 0.0},
    {UNIT_CATEGORY_MASS, "tonne", "t", 1e3, 0.0},
    {UNIT_CATEGORY_MASS, "ounce", "oz", 0.028349523125, 0.0},
    {UNIT_CATEGORY_MASS, "pound", "lb", 0.45359237, 0.0},
    {UNIT_CATEGORY_MASS, "stone", "st", 6.35029318, 0.0},

    {UNIT_CATEGORY_TIME, "microsecond", "us", 1e-6, 0.0},
    {UNIT_CATEGORY_TIME, "millisecond", "ms", 1e-3, 0.0},
    {UNIT_CATEGORY_TIME, "second", "s", 1.0, 0.0},
    {UNIT_CATEGORY_TIME, "minute", "min", 60.0, 0.0},
    {UNIT_CATEGORY_TIME, "hour", "h", 3600.0, 0.0},
    {UNIT_CATEGORY_TIME, "day", "d", 86400.0, 0.0},
    {UNIT_CATEGORY_TIME, "week", "wk", 604800.0, 0.0},

    {UNIT_CATEGORY_TEMPERATURE, "kelvin", "K", 1.0, 0.0},
    {UNIT_CATEGORY_TEMPERATURE, "Celsius", "degC", 1.0, 273.15},
    {UNIT_CATEGORY_TEMPERATURE, "Fahrenheit", "degF", 5.0 / 9.0,
     255.37222222222222222},
    {UNIT_CATEGORY_TEMPERATURE, "Rankine", "degR", 5.0 / 9.0, 0.0},

    {UNIT_CATEGORY_ANGLE, "radian", "rad", 1.0, 0.0},
    {UNIT_CATEGORY_ANGLE, "degree", "deg", PI_VALUE / 180.0, 0.0},
    {UNIT_CATEGORY_ANGLE, "gradian", "gon", PI_VALUE / 200.0, 0.0},
    {UNIT_CATEGORY_ANGLE, "turn", "turn", 2.0 * PI_VALUE, 0.0},

    {UNIT_CATEGORY_PRESSURE, "pascal", "Pa", 1.0, 0.0},
    {UNIT_CATEGORY_PRESSURE, "kilopascal", "kPa", 1e3, 0.0},
    {UNIT_CATEGORY_PRESSURE, "megapascal", "MPa", 1e6, 0.0},
    {UNIT_CATEGORY_PRESSURE, "bar", "bar", 1e5, 0.0},
    {UNIT_CATEGORY_PRESSURE, "millibar", "mbar", 100.0, 0.0},
    {UNIT_CATEGORY_PRESSURE, "psi", "psi", 6894.757293168, 0.0},
    {UNIT_CATEGORY_PRESSURE, "atmosphere", "atm", 101325.0, 0.0},
    {UNIT_CATEGORY_PRESSURE, "torr", "Torr", 101325.0 / 760.0, 0.0},

    {UNIT_CATEGORY_ENERGY, "joule", "J", 1.0, 0.0},
    {UNIT_CATEGORY_ENERGY, "kilojoule", "kJ", 1e3, 0.0},
    {UNIT_CATEGORY_ENERGY, "watt hour", "Wh", 3600.0, 0.0},
    {UNIT_CATEGORY_ENERGY, "kilowatt hour", "kWh", 3.6e6, 0.0},
    {UNIT_CATEGORY_ENERGY, "calorie", "cal", 4.184, 0.0},
    {UNIT_CATEGORY_ENERGY, "kilocalorie", "kcal", 4184.0, 0.0},
    {UNIT_CATEGORY_ENERGY, "electronvolt", "eV", 1.602176634e-19, 0.0},
    {UNIT_CATEGORY_ENERGY, "BTU IT", "BTU", 1055.05585262, 0.0},

    {UNIT_CATEGORY_POWER, "milliwatt", "mW", 1e-3, 0.0},
    {UNIT_CATEGORY_POWER, "watt", "W", 1.0, 0.0},
    {UNIT_CATEGORY_POWER, "kilowatt", "kW", 1e3, 0.0},
    {UNIT_CATEGORY_POWER, "megawatt", "MW", 1e6, 0.0},
    {UNIT_CATEGORY_POWER, "horsepower", "hp", 745.69987158227022, 0.0},
    {UNIT_CATEGORY_POWER, "BTU per hour", "BTU/h", 0.2930710701722222, 0.0},
};

static const physical_constant_t constants[] = {
    {"speed of light", "c", 299792458.0, "m/s", "BIPM SI exact"},
    {"Planck constant", "h", 6.62607015e-34, "J s", "BIPM SI exact"},
    {"reduced Planck", "hbar", 1.054571817e-34, "J s", "CODATA 2022"},
    {"elementary charge", "e", 1.602176634e-19, "C", "BIPM SI exact"},
    {"Boltzmann constant", "k", 1.380649e-23, "J/K", "BIPM SI exact"},
    {"Avogadro constant", "NA", 6.02214076e23, "1/mol", "BIPM SI exact"},
    {"molar gas constant", "R", 8.314462618, "J/(mol K)", "CODATA 2022"},
    {"gravitation constant", "G", 6.67430e-11, "m3/(kg s2)",
     "CODATA 2022"},
    {"standard gravity", "g0", 9.80665, "m/s2", "BIPM SI exact"},
    {"electron mass", "me", 9.1093837139e-31, "kg", "CODATA 2022"},
    {"proton mass", "mp", 1.67262192595e-27, "kg", "CODATA 2022"},
    {"Stefan-Boltzmann", "sigma", 5.670374419e-8, "W/(m2 K4)",
     "CODATA 2022"},
};

const char *unit_engine_category_name(unit_category_t category) {
    if (category < 0 || category >= UNIT_CATEGORY_COUNT) return "INVALID";
    return category_names[category];
}

size_t unit_engine_unit_count(unit_category_t category) {
    size_t count = 0;
    for (size_t i = 0; i < sizeof units / sizeof units[0]; ++i) {
        if (units[i].category == category) count++;
    }
    return count;
}

const unit_definition_t *unit_engine_unit(unit_category_t category,
                                          size_t index) {
    for (size_t i = 0; i < sizeof units / sizeof units[0]; ++i) {
        if (units[i].category != category) continue;
        if (!index) return &units[i];
        index--;
    }
    return NULL;
}

unit_status_t unit_engine_convert(double value,
                                  const unit_definition_t *from,
                                  const unit_definition_t *to,
                                  double *result) {
    if (!from || !to || !result || from->scale == 0.0 || to->scale == 0.0) {
        return UNIT_STATUS_INVALID_UNIT;
    }
    if (from->category != to->category) return UNIT_STATUS_INCOMPATIBLE;
    if (!isfinite(value)) return UNIT_STATUS_NONFINITE;
    double base = value * from->scale + from->offset;
    if (!isfinite(base)) return UNIT_STATUS_NONFINITE;
    if (from->category == UNIT_CATEGORY_TEMPERATURE && base < 0.0) {
        return UNIT_STATUS_DOMAIN;
    }
    double converted = (base - to->offset) / to->scale;
    if (!isfinite(converted)) return UNIT_STATUS_NONFINITE;
    *result = converted;
    return UNIT_STATUS_OK;
}

const char *unit_engine_status_text(unit_status_t status) {
    switch (status) {
        case UNIT_STATUS_OK: return "OK";
        case UNIT_STATUS_INVALID_UNIT: return "INVALID UNIT";
        case UNIT_STATUS_INCOMPATIBLE: return "INCOMPATIBLE";
        case UNIT_STATUS_DOMAIN: return "BELOW ABS ZERO";
        case UNIT_STATUS_NONFINITE: return "NUMBER RANGE";
        default: return "UNIT ERROR";
    }
}

size_t unit_engine_constant_count(void) {
    return sizeof constants / sizeof constants[0];
}

const physical_constant_t *unit_engine_constant(size_t index) {
    return index < unit_engine_constant_count() ? &constants[index] : NULL;
}
