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

    puts("BASIC engine tests passed");
    return 0;
}
