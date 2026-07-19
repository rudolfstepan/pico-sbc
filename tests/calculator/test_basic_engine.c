#include "basic_engine.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static basic_status_t run_to_stop(basic_engine_t *engine) {
    basic_status_t status = BASIC_STATUS_OK;
    while (engine->state == BASIC_RUN_RUNNING) {
        status = basic_engine_step(engine);
    }
    return status;
}

static int load_example(basic_engine_t *engine, const char *filename) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s", BASIC_EXAMPLES_DIR, filename);
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "cannot open BASIC example: %s\n", path);
        return 1;
    }

    char line[128];
    while (fgets(line, sizeof line, file)) {
        size_t length = strcspn(line, "\r\n");
        line[length] = '\0';
        if (!line[0]) continue;
        basic_status_t status = basic_engine_store_line(engine, line);
        if (status != BASIC_STATUS_OK) {
            fprintf(stderr, "%s: %s: %s\n", filename, line,
                    basic_status_text(status));
            fclose(file);
            return 1;
        }
    }
    if (ferror(file)) {
        fprintf(stderr, "cannot read BASIC example: %s\n", path);
        fclose(file);
        return 1;
    }
    fclose(file);
    return 0;
}

static int run_example(const char *filename, const char *input,
                       const char *expected_last_output) {
    basic_engine_t engine;
    basic_engine_init(&engine);
    if (load_example(&engine, filename)) return 1;
    if (basic_engine_run(&engine) != BASIC_STATUS_OK) return 1;

    bool input_submitted = false;
    while (engine.state == BASIC_RUN_RUNNING ||
           engine.state == BASIC_RUN_INPUT) {
        if (engine.state == BASIC_RUN_INPUT) {
            if (!input || input_submitted ||
                basic_engine_submit_input(&engine, input) != BASIC_STATUS_OK) {
                fprintf(stderr, "%s: unexpected INPUT state\n", filename);
                return 1;
            }
            input_submitted = true;
        } else if (basic_engine_step(&engine) != BASIC_STATUS_OK) {
            fprintf(stderr, "%s: execution failed: %s\n", filename,
                    basic_status_text(engine.status));
            return 1;
        }
    }
    if (engine.state != BASIC_RUN_FINISHED || !engine.output_count) return 1;
    const char *actual = engine.output[engine.output_count - 1u];
    if (strcmp(actual, expected_last_output) != 0) {
        fprintf(stderr, "%s: expected '%s', got '%s'\n", filename,
                expected_last_output, actual);
        return 1;
    }
    return 0;
}

int main(void) {
    basic_engine_t engine;
    basic_engine_init(&engine);
    CHECK(basic_engine_store_line(&engine, "30 END") == BASIC_STATUS_OK);
    CHECK(basic_engine_store_line(&engine, "10 LET A=1") == BASIC_STATUS_OK);
    CHECK(basic_engine_store_line(&engine, "20 PRINT A") == BASIC_STATUS_OK);
    CHECK(engine.program.count == 3);
    CHECK(engine.program.lines[0].number == 10);
    CHECK(basic_engine_store_line(&engine, "20 PRINT A+1") == BASIC_STATUS_OK);
    CHECK(strcmp(engine.program.lines[1].text, "PRINT A+1") == 0);
    CHECK(basic_engine_store_line(&engine, "20") == BASIC_STATUS_OK);
    CHECK(engine.program.count == 2);
    CHECK(basic_program_is_valid(&engine.program));

    basic_engine_clear_program(&engine);
    const char *loop_program[] = {
        "10 LET A=1", "20 PRINT \"COUNT\"", "30 PRINT A",
        "40 A=A+1", "50 IF A<=3 THEN 30", "60 END"
    };
    for (size_t i = 0; i < sizeof loop_program / sizeof loop_program[0]; ++i) {
        CHECK(basic_engine_store_line(&engine, loop_program[i]) == BASIC_STATUS_OK);
    }
    CHECK(basic_engine_run(&engine) == BASIC_STATUS_OK);
    CHECK(run_to_stop(&engine) == BASIC_STATUS_OK);
    CHECK(engine.state == BASIC_RUN_FINISHED);
    CHECK(engine.output_count == 4);
    CHECK(strcmp(engine.output[0], "COUNT") == 0);
    CHECK(strcmp(engine.output[1], "1") == 0);
    CHECK(strcmp(engine.output[3], "3") == 0);

    basic_engine_clear_program(&engine);
    CHECK(basic_engine_store_line(&engine, "10 INPUT B") == BASIC_STATUS_OK);
    CHECK(basic_engine_store_line(&engine, "20 PRINT B*2") == BASIC_STATUS_OK);
    CHECK(basic_engine_store_line(&engine, "30 END") == BASIC_STATUS_OK);
    CHECK(basic_engine_run(&engine) == BASIC_STATUS_OK);
    CHECK(basic_engine_step(&engine) == BASIC_STATUS_OK);
    CHECK(engine.state == BASIC_RUN_INPUT);
    CHECK(basic_engine_submit_input(&engine, "21") == BASIC_STATUS_OK);
    CHECK(run_to_stop(&engine) == BASIC_STATUS_OK);
    CHECK(strcmp(engine.output[1], "42") == 0);

    basic_engine_clear_program(&engine);
    CHECK(basic_engine_store_line(&engine, "10 FOR I=3 TO 1 STEP -1") ==
          BASIC_STATUS_OK);
    CHECK(basic_engine_store_line(&engine, "20 PRINT I") == BASIC_STATUS_OK);
    CHECK(basic_engine_store_line(&engine, "30 NEXT I") == BASIC_STATUS_OK);
    CHECK(basic_engine_store_line(&engine, "40 END") == BASIC_STATUS_OK);
    CHECK(basic_engine_run(&engine) == BASIC_STATUS_OK);
    CHECK(run_to_stop(&engine) == BASIC_STATUS_OK);
    CHECK(engine.output_count == 3);
    CHECK(strcmp(engine.output[0], "3") == 0);
    CHECK(strcmp(engine.output[2], "1") == 0);

    basic_engine_clear_program(&engine);
    CHECK(basic_engine_store_line(&engine, "10 GOTO 99") == BASIC_STATUS_OK);
    CHECK(basic_engine_run(&engine) == BASIC_STATUS_OK);
    CHECK(basic_engine_step(&engine) == BASIC_STATUS_NO_TARGET);
    CHECK(engine.state == BASIC_RUN_ERROR && engine.error_line == 10);

    basic_engine_clear_program(&engine);
    CHECK(basic_engine_store_line(&engine, "10 GOTO 10") == BASIC_STATUS_OK);
    CHECK(basic_engine_run(&engine) == BASIC_STATUS_OK);
    CHECK(run_to_stop(&engine) == BASIC_STATUS_LIMIT);
    CHECK(engine.state == BASIC_RUN_ERROR);

    basic_engine_clear_program(&engine);
    CHECK(basic_engine_store_line(&engine, "10 PRINT 1e-310") == BASIC_STATUS_OK);
    CHECK(basic_engine_store_line(&engine, "20 END") == BASIC_STATUS_OK);
    CHECK(basic_engine_run(&engine) == BASIC_STATUS_OK);
    CHECK(run_to_stop(&engine) == BASIC_STATUS_OK);
    CHECK(isfinite(strtod(engine.output[0], NULL)));

    static const struct {
        const char *filename;
        const char *input;
        const char *expected_last_output;
    } examples[] = {
        {"01_hello.bas", NULL, "READY"},
        {"02_arithmetic.bas", NULL, "42"},
        {"03_for_loop.bas", NULL, "25"},
        {"04_input_branch.bas", "-9", "9"},
        {"05_countdown.bas", NULL, "START"},
        {"06_sum_goto.bas", NULL, "55"},
        {"07_factorial.bas", "5", "120"},
        {"08_trigonometry.bas", NULL, "1"},
        {"09_mandelbrot_text.bas", NULL, "100000100"},
    };
    for (size_t i = 0; i < sizeof examples / sizeof examples[0]; ++i) {
        CHECK(run_example(examples[i].filename, examples[i].input,
                          examples[i].expected_last_output) == 0);
    }

    puts("BASIC engine tests passed");
    return 0;
}
