#include "high_precision_engine.h"

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

static int expect_prefix(const char *expression, bool degrees,
                         const char *prefix) {
    high_precision_result_t result;
    int error = 0;
    high_precision_status_t status = high_precision_engine_evaluate(
        expression, "0", NULL, degrees, &result, &error);
    if (status != HIGH_PRECISION_STATUS_OK ||
        strncmp(result.text, prefix, strlen(prefix)) != 0) {
        fprintf(stderr, "%s -> %s '%s' error=%d\n", expression,
                high_precision_status_text(status),
                status == HIGH_PRECISION_STATUS_OK ? result.text : "",
                error);
        return 1;
    }
    return 0;
}

int main(void) {
    CHECK(expect_prefix(
        "pi", false,
        "3.14159265358979323846264338327950288419716939937510") == 0);
    CHECK(expect_prefix(
        "e", false,
        "2.71828182845904523536028747135266249775724709369995") == 0);
    CHECK(expect_prefix(
        "tau", false,
        "6.28318530717958647692528676655900576839433879875021") == 0);
    CHECK(expect_prefix(
        "phi", false,
        "1.61803398874989484820458683436563811772030917980576") == 0);
    CHECK(expect_prefix(
        "sqrt(2)", false,
        "1.41421356237309504880168872420969807856967187537694") == 0);
    CHECK(expect_prefix(
        "sin(1)", false,
        "0.84147098480789650665250232163029899962256306079837") == 0);
    CHECK(expect_prefix(
        "cos(1)", false,
        "0.54030230586813971740093660744297660373231042061792") == 0);
    CHECK(expect_prefix(
        "tan(1)", false,
        "1.5574077246549022305069748074583601730872507723815") == 0);
    CHECK(expect_prefix(
        "ln(2)", false,
        "0.69314718055994530941723212145817656807550013436025") == 0);
    CHECK(expect_prefix(
        "exp(1)", false,
        "2.71828182845904523536028747135266249775724709369995") == 0);
    CHECK(expect_prefix("sin(30)", true, "0.5") == 0);
    CHECK(expect_prefix("asin(0.5)", true, "30") == 0);
    CHECK(expect_prefix("acos(0.5)", true, "60") == 0);
    CHECK(expect_prefix("atan(1)", true, "45") == 0);
    CHECK(expect_prefix("atan2(1,1)", true, "45") == 0);
    CHECK(expect_prefix("log(100)", false, "2") == 0);
    CHECK(expect_prefix("sinh(1)", false,
                        "1.17520119364380145688238185059560081515571798133409") == 0);
    CHECK(expect_prefix("cosh(1)", false,
                        "1.54308063481524377847790562075706168260152911236586") == 0);
    CHECK(expect_prefix("tanh(1)", false,
                        "0.76159415595576488811945828260479359041276859725793") == 0);
    CHECK(expect_prefix("2^0.5", false,
                        "1.41421356237309504880168872420969807856967187537694") == 0);
    CHECK(expect_prefix("pow(2,0.5)", false,
                        "1.41421356237309504880168872420969807856967187537694") == 0);
    CHECK(expect_prefix("abs(-4)+floor(2.9)+ceil(2.1)", false, "9") == 0);
    CHECK(expect_prefix("10%3", false, "1") == 0);
    CHECK(expect_prefix("fac(25)", false, "15511210043330985984000000") == 0);
    CHECK(expect_prefix("ncr(100,50)", false,
                        "100891344545564193334812497256") == 0);
    CHECK(expect_prefix("npr(20,5)", false, "1860480") == 0);

    calculator_symbols_t symbols;
    calculator_symbols_init(&symbols);
    calculator_symbols_set_variable(&symbols, 0, 2.0);
    calculator_symbols_set_function(&symbols, 0, "sqrt(x)+A");
    high_precision_result_t result;
    int error = 0;
    CHECK(high_precision_engine_evaluate(
              "f1(2)", "0", &symbols, false, &result, &error) ==
          HIGH_PRECISION_STATUS_OK);
    CHECK(strncmp(result.text,
                  "3.41421356237309504880168872420969807856967187537694",
                  52u) == 0);
    CHECK(calculator_symbols_set_variable_precise(
        &symbols, 0, 2.0,
        "2.0000000000000000000000000000000000000000000001"));
    CHECK(high_precision_engine_evaluate(
              "A+0.0000000000000000000000000000000000000000000001",
              "0", &symbols, false, &result, &error) ==
          HIGH_PRECISION_STATUS_OK);
    CHECK(strcmp(result.text,
                 "2.0000000000000000000000000000000000000000000002") == 0);

    CHECK(high_precision_engine_evaluate(
              "sqrt(-1)", "0", NULL, false, &result, &error) ==
          HIGH_PRECISION_STATUS_DOMAIN);
    CHECK(high_precision_engine_evaluate(
              "1/0", "0", NULL, false, &result, &error) ==
          HIGH_PRECISION_STATUS_DIV_ZERO);
    CHECK(high_precision_engine_evaluate(
              "sin(", "0", NULL, false, &result, &error) ==
          HIGH_PRECISION_STATUS_SYNTAX);

    puts("high precision engine tests passed");
    return 0;
}
