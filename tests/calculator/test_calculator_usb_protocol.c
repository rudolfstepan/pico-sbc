#include "calculator_usb_protocol.h"

#include "calculator_symbols.h"
#include "graph_model.h"
#include "statistics_engine.h"

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

static const char *run(calculator_usb_context_t *context, const char *command,
                       calculator_usb_effect_t *effect) {
    static char response[CALCULATOR_USB_RESPONSE_CAPACITY];
    calculator_usb_execute(context, command, response, sizeof response,
                           effect);
    return response;
}

int main(void) {
    calculator_persisted_state_t state;
    expression_editor_t editor;
    basic_engine_t basic_engine;
    memset(&state, 0, sizeof state);
    snprintf(state.ans_text, sizeof state.ans_text, "0");
    state.degrees = true;
    state.page = PAGE_BASIC;
    calculator_symbols_init(&state.symbols);
    graph_model_init(&state.graph);
    statistics_engine_init(&state.statistics);
    expression_editor_init(&editor);
    basic_engine_init(&basic_engine);
    calculator_usb_context_t context = {
        .state = &state,
        .editor = &editor,
        .basic_engine = &basic_engine,
    };
    calculator_usb_effect_t effect;

    CHECK(strcmp(run(&context, "PING", &effect), "OK PONG") == 0);
    CHECK(!effect.changed);
    CHECK(strstr(run(&context, "INFO", &effect), "protocol=3") != NULL);
    CHECK(strstr(run(&context, "DIAG", &effect), "mode=1") != NULL);

    CHECK(strcmp(run(&context, "SET EXPR 6*7", &effect),
                 "OK EXPR\t6*7") == 0);
    CHECK(effect.changed && strcmp(editor.text, "6*7") == 0);
    CHECK(strcmp(run(&context, "GET EXPR", &effect),
                 "OK EXPR\t6*7") == 0);
    CHECK(strcmp(run(&context, "EVAL", &effect),
                 "OK RESULT\t42") == 0);
    CHECK(effect.changed && effect.evaluated && state.ans == 42.0);
    CHECK(state.history_count == 1);
    CHECK(strcmp(run(&context, "GET HISTORY", &effect),
                 "OK HISTORY\t1") == 0);
    CHECK(strstr(run(&context, "GET HISTORY 0", &effect),
                 "\t42\t6*7\t42") != NULL);

    const char *precise_expression =
        "EVAL 1.000000000000000000000000000000000000000000001*2";
    CHECK(strcmp(run(&context, precise_expression, &effect),
        "OK RESULT\t2.000000000000000000000000000000000000000000002") == 0);
    CHECK(strcmp(run(&context, "GET RESULT", &effect),
        "OK RESULT\t2.000000000000000000000000000000000000000000002") == 0);
    CHECK(strcmp(state.history[state.history_count - 1u].result,
        "2.000000000000000000000000000000000000000000002") == 0);
    CHECK(strcmp(run(&context,
        "EVAL ans+0.000000000000000000000000000000000000000000001",
        &effect),
        "OK RESULT\t2.000000000000000000000000000000000000000000003") == 0);

    CHECK(strcmp(run(&context, "SET VAR a 5", &effect),
                 "OK VAR\tA\t5") == 0);
    CHECK(strcmp(run(&context, "GET VAR A", &effect),
                 "OK VAR\tA\t5") == 0);
    CHECK(strcmp(run(&context, "SET FUNC F1 x+A", &effect),
                 "OK FUNC\tF1\tx+A") == 0);
    CHECK(strcmp(state.graph.functions[0].expression, "x+A") == 0);
    CHECK(strcmp(run(&context, "GET FUNC f1", &effect),
                 "OK FUNC\tF1\tx+A") == 0);
    CHECK(strcmp(run(&context, "EVAL f1(2)", &effect),
                 "OK RESULT\t7") == 0);
    CHECK(strncmp(run(&context, "SET FUNC F1 f1(x)", &effect),
                  "ERR INVALID_FUNC", 16) == 0);
    CHECK(strcmp(state.symbols.functions[0], "x+A") == 0);

    CHECK(strcmp(run(&context, "STAT MODE 2", &effect),
                 "OK STATS\t2\t0") == 0);
    CHECK(strcmp(run(&context, "STAT ADD 1 3", &effect),
                 "OK STATS\t2\t1") == 0);
    CHECK(strcmp(run(&context, "STAT ADD 2 5", &effect),
                 "OK STATS\t2\t2") == 0);
    CHECK(strcmp(run(&context, "GET STATS", &effect),
                 "OK STATS\t2\t2") == 0);
    CHECK(strcmp(run(&context, "GET STATS 1", &effect),
                 "OK STATS\t1\t2\t5") == 0);
    CHECK(strcmp(run(&context, "STAT ADD 3", &effect),
                 "ERR ARGUMENT STAT ADD x y") == 0);
    CHECK(strcmp(run(&context, "STAT CLEAR", &effect),
                 "OK STATS\t2\t0") == 0);
    CHECK(strcmp(run(&context, "STAT MODE 1", &effect),
                 "OK STATS\t1\t0") == 0);
    CHECK(strcmp(run(&context, "STAT ADD -2.5", &effect),
                 "OK STATS\t1\t1") == 0);
    CHECK(strcmp(run(&context, "GET STATS 0", &effect),
                 "OK STATS\t0\t-2.5") == 0);

    CHECK(strcmp(run(&context, "BASIC LINE 20 END", &effect),
                 "OK BASIC\t1") == 0);
    CHECK(effect.changed && effect.persistent_changed &&
          effect.basic_program_changed);
    CHECK(strcmp(run(&context, "BASIC LINE 10 PRINT \"USB\"", &effect),
                 "OK BASIC\t2") == 0);
    CHECK(strcmp(run(&context, "GET BASIC", &effect),
                 "OK BASIC\t2") == 0);
    CHECK(strcmp(run(&context, "GET BASIC 0", &effect),
                 "OK BASIC\t0\t10\tPRINT \"USB\"") == 0);
    CHECK(strcmp(run(&context, "BASIC RUN", &effect),
                 "OK BASIC_RUN\tRUNNING") == 0);
    CHECK(!effect.changed && effect.basic_runtime_changed);
    CHECK(strstr(run(&context, "GET BASIC STATUS", &effect),
                 "state=RUNNING") != NULL);
    while (basic_engine.state == BASIC_RUN_RUNNING) {
        basic_engine_step(&basic_engine);
    }
    CHECK(strstr(run(&context, "GET BASIC STATUS", &effect),
                 "state=FINISHED") != NULL);
    CHECK(strcmp(run(&context, "GET BASIC OUTPUT", &effect),
                 "OK BASIC_OUTPUT\t1") == 0);
    CHECK(strcmp(run(&context, "GET BASIC OUTPUT 0", &effect),
                 "OK BASIC_OUTPUT\t0\tUSB") == 0);

    CHECK(strcmp(run(&context, "BASIC CLEAR", &effect),
                 "OK BASIC\t0") == 0);
    CHECK(strcmp(run(&context, "BASIC LINE 10 INPUT A", &effect),
                 "OK BASIC\t1") == 0);
    CHECK(strcmp(run(&context, "BASIC LINE 20 PRINT A*2", &effect),
                 "OK BASIC\t2") == 0);
    CHECK(strcmp(run(&context, "BASIC RUN", &effect),
                 "OK BASIC_RUN\tRUNNING") == 0);
    CHECK(basic_engine_step(&basic_engine) == BASIC_STATUS_OK);
    CHECK(strstr(run(&context, "GET BASIC STATUS", &effect),
                 "state=INPUT") != NULL);
    CHECK(strcmp(run(&context, "BASIC INPUT 21", &effect),
                 "OK BASIC_INPUT\tRUNNING") == 0);
    while (basic_engine.state == BASIC_RUN_RUNNING) {
        basic_engine_step(&basic_engine);
    }
    CHECK(strcmp(run(&context, "GET BASIC OUTPUT 1", &effect),
                 "OK BASIC_OUTPUT\t1\t42") == 0);
    CHECK(strcmp(run(&context, "BASIC STOP", &effect),
                 "OK BASIC_RUN\tSTOPPED") == 0);

    CHECK(strncmp(run(&context, "EVAL sin(", &effect), "ERR PARSE", 9) == 0);
    CHECK(strcmp(run(&context, "SET VAR A nan", &effect),
                 "ERR ARGUMENT VAR A-F value") == 0);
    CHECK(strcmp(run(&context, "GET HISTORY 99", &effect),
                 "ERR INDEX HISTORY") == 0);
    CHECK(strcmp(run(&context, "GET BASIC OUTPUT 99", &effect),
                 "ERR INDEX BASIC_OUTPUT") == 0);
    CHECK(strncmp(run(&context, "BOGUS", &effect),
                  "ERR UNKNOWN_COMMAND", 19) == 0);
    CHECK(strcmp(run(&context, "PING extra", &effect),
                 "ERR ARGUMENT PING") == 0);

    char too_long[CALCULATOR_USB_LINE_CAPACITY + 1];
    memset(too_long, 'x', sizeof too_long - 1);
    too_long[sizeof too_long - 1] = '\0';
    CHECK(strcmp(run(&context, too_long, &effect),
                 "ERR LINE_TOO_LONG") == 0);
    CHECK(strcmp(run(&context, "PING\t", &effect),
                 "ERR INVALID_CHAR") == 0);

    calculator_usb_line_reader_t reader;
    calculator_usb_line_reader_init(&reader);
    const char *ping = "PINX\bG\r\n";
    calculator_usb_line_status_t line_status = CALCULATOR_USB_LINE_NONE;
    for (size_t i = 0; ping[i]; ++i) {
        line_status = calculator_usb_line_reader_feed(&reader, ping[i]);
    }
    CHECK(line_status == CALCULATOR_USB_LINE_READY);
    CHECK(strcmp(reader.line, "PING") == 0);

    calculator_usb_line_reader_init(&reader);
    for (size_t i = 0; i < CALCULATOR_USB_LINE_CAPACITY; ++i) {
        calculator_usb_line_reader_feed(&reader, 'x');
    }
    CHECK(calculator_usb_line_reader_feed(&reader, '\n') ==
          CALCULATOR_USB_LINE_TOO_LONG);
    calculator_usb_line_reader_init(&reader);
    calculator_usb_line_reader_feed(&reader, 1);
    CHECK(calculator_usb_line_reader_feed(&reader, '\n') ==
          CALCULATOR_USB_LINE_INVALID);

    puts("calculator USB protocol tests passed");
    return 0;
}
