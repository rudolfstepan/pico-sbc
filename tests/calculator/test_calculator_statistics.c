#include "calculator_statistics.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static void press(calculator_statistics_t *stats, const char *token,
                  double ans, char *message) {
    calculator_statistics_activate(stats, token, ans, message, 32);
}

static void add_pair(calculator_statistics_t *stats, double x, double y,
                     char *message) {
    press(stats, "ANS", x, message);
    press(stats, "XY", 0.0, message);
    press(stats, "ANS", y, message);
    press(stats, "ADD", 0.0, message);
}

int main(void) {
    calculator_statistics_t stats;
    char message[32];
    calculator_statistics_init(&stats);
    press(&stats, "ANS", 2.5, message);
    press(&stats, "ADD", 0.0, message);
    CHECK(stats.dataset.count == 1 && stats.dataset.x[0] == 2.5);

    press(&stats, "2VAR", 0.0, message);
    CHECK(stats.dataset.two_variable && stats.dataset.count == 0);
    add_pair(&stats, 1.0, 3.0, message);
    add_pair(&stats, 2.0, 5.0, message);
    CHECK(stats.dataset.count == 2);
    press(&stats, "REGRESSION", 0.0, message);
    CHECK(stats.view == STATISTICS_VIEW_REGRESSION);

    press(&stats, "PREV", 0.0, message);
    press(&stats, "EDIT", 0.0, message);
    CHECK(strcmp(stats.x_editor.text, "1") == 0);
    CHECK(strcmp(stats.y_editor.text, "3") == 0);
    press(&stats, "ANS", 4.0, message);
    press(&stats, "ADD", 0.0, message);
    CHECK(stats.dataset.count == 2);
    CHECK(stats.dataset.x[0] == 4.0 && stats.dataset.y[0] == 3.0);
    press(&stats, "EDIT", 0.0, message);
    press(&stats, "NEXT", 0.0, message);
    CHECK(!stats.editing && stats.x_editor.length == 0 &&
          stats.selected == 1);
    press(&stats, "PREV", 0.0, message);
    press(&stats, "DROP", 0.0, message);
    CHECK(stats.dataset.count == 1);
    press(&stats, "CLEAR", 0.0, message);
    CHECK(stats.dataset.count == 0 && stats.dataset.two_variable);

    puts("calculator statistics tests passed");
    return 0;
}
