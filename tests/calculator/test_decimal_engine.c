#include "decimal_engine.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int expect_result(const char *expression, const char *ans,
                         const char *expected, bool exact) {
    decimal_result_t result;
    int error = 0;
    decimal_status_t status = decimal_engine_evaluate(
        expression, ans, &result, &error);
    if (status != DECIMAL_STATUS_OK || strcmp(result.text, expected) != 0 ||
        result.exact != exact) {
        fprintf(stderr, "%s -> %s '%s' exact=%d error=%d\n", expression,
                decimal_status_text(status),
                status == DECIMAL_STATUS_OK ? result.text : "",
                status == DECIMAL_STATUS_OK ? result.exact : 0, error);
        return 1;
    }
    return 0;
}

int main(void) {
    CHECK(expect_result("0.1+0.2", NULL, "0.3", true) == 0);
    CHECK(expect_result("9007199254740993+1", NULL,
                        "9007199254740994", true) == 0);
    CHECK(expect_result(
        "1.000000000000000000000000000000000000000000001*2", NULL,
        "2.000000000000000000000000000000000000000000002", true) == 0);
    CHECK(expect_result("(1.25-0.25)^3", NULL, "1", true) == 0);
    CHECK(expect_result("2^3^2", NULL, "512", true) == 0);
    CHECK(expect_result("2^-3", NULL, "0.125", true) == 0);
    CHECK(expect_result("10%3", NULL, "1", true) == 0);
    CHECK(expect_result("1/8", NULL, "0.125", true) == 0);
    CHECK(expect_result("1/3", NULL,
        "0.33333333333333333333333333333333333333333333333333333333333333333333333333333333",
        false) == 0);
    CHECK(expect_result("1/2^115", NULL,
        "2.4074124304840448163199724282311591481726270602692352440499234944581985473632812e-35",
        false) == 0);
    CHECK(expect_result("3/2^115", NULL,
        "7.2222372914521344489599172846934774445178811808077057321497704833745956420898438e-35",
        false) == 0);
    CHECK(expect_result("ans+0.000000000000000000000000000000000000000000001",
                        "1.000000000000000000000000000000000000000000001",
                        "1.000000000000000000000000000000000000000000002",
                        true) == 0);

    decimal_result_t result;
    int error = 0;
    CHECK(decimal_engine_evaluate("sin(1)", NULL, &result, &error) ==
          DECIMAL_STATUS_UNSUPPORTED);
    CHECK(decimal_engine_evaluate("1/0", NULL, &result, &error) ==
          DECIMAL_STATUS_DIV_ZERO);
    CHECK(decimal_engine_evaluate("2+(", NULL, &result, &error) ==
          DECIMAL_STATUS_SYNTAX);
    CHECK(decimal_engine_evaluate(
        "99999999999999999999999999999999999999999*"
        "99999999999999999999999999999999999999999",
        NULL, &result, &error) == DECIMAL_STATUS_PRECISION);

    uint8_t packed[DECIMAL_ENGINE_PACKED_CAPACITY];
    char unpacked[DECIMAL_ENGINE_TEXT_CAPACITY];
    const char *packed_value =
        "2.000000000000000000000000000000000000000000002";
    CHECK(decimal_engine_pack_text(packed_value, packed, sizeof packed));
    CHECK(decimal_engine_unpack_text(packed, sizeof packed,
                                     unpacked, sizeof unpacked));
    CHECK(strcmp(unpacked, packed_value) == 0);
    packed[4] = 0x0fu;
    CHECK(!decimal_engine_unpack_text(packed, sizeof packed,
                                      unpacked, sizeof unpacked));

    puts("decimal engine tests passed");
    return 0;
}
