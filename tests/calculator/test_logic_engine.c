#include "logic_engine.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static bool equivalent(const logic_program_t *first,
                       const logic_program_t *second) {
    for (uint8_t assignment = 0; assignment < 64; ++assignment) {
        if (logic_engine_evaluate(first, assignment) !=
            logic_engine_evaluate(second, assignment)) return false;
    }
    return true;
}

static int check_generated_form(const logic_program_t *original, bool dnf,
                                bool canonical) {
    char form[LOGIC_FORM_CAPACITY];
    logic_program_t generated;
    int error = 0;
    logic_status_t status = canonical
        ? logic_engine_format_canonical(original, dnf, form, sizeof form)
        : logic_engine_format_simplified(original, dnf, form, sizeof form);
    CHECK(status == LOGIC_STATUS_OK);
    CHECK(logic_engine_compile(form, &generated, &error) == LOGIC_STATUS_OK);
    CHECK(equivalent(original, &generated));
    return 0;
}

int main(void) {
    logic_program_t program;
    logic_program_t original;
    int error = 0;
    CHECK(logic_engine_compile("A & !B | C", &program, &error) ==
          LOGIC_STATUS_OK);
    CHECK(program.variable_mask == 0x07);
    CHECK(!logic_engine_evaluate(&program, 0));
    CHECK(logic_engine_evaluate(&program, 1));
    CHECK(logic_engine_evaluate(&program, 4));
    CHECK(!logic_engine_evaluate(&program, 3));
    CHECK(logic_engine_truth_row_count(&program) == 8);

    CHECK(logic_engine_compile("A NAND B", &program, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_engine_evaluate(&program, 0));
    CHECK(!logic_engine_evaluate(&program, 3));
    CHECK(logic_engine_compile("A NOR B", &program, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_engine_evaluate(&program, 0));
    CHECK(!logic_engine_evaluate(&program, 1));
    CHECK(logic_engine_compile("A XNOR B", &program, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_engine_evaluate(&program, 0));
    CHECK(!logic_engine_evaluate(&program, 1));
    CHECK(logic_engine_compile("A -> B", &program, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_engine_evaluate(&program, 0));
    CHECK(logic_engine_evaluate(&program, 2));
    CHECK(!logic_engine_evaluate(&program, 1));
    CHECK(logic_engine_evaluate(&program, 3));
    CHECK(logic_engine_compile("A IMPLIES B IFF !A OR B", &program,
                               &error) == LOGIC_STATUS_OK);
    CHECK(logic_engine_evaluate(&program, 0));
    CHECK(logic_engine_evaluate(&program, 1));
    CHECK(logic_engine_evaluate(&program, 2));
    CHECK(logic_engine_evaluate(&program, 3));

    CHECK(logic_engine_compile("A^B^C^D", &original, &error) ==
          LOGIC_STATUS_OK);
    CHECK(check_generated_form(&original, true, true) == 0);
    CHECK(check_generated_form(&original, false, true) == 0);
    CHECK(check_generated_form(&original, true, false) == 0);
    CHECK(check_generated_form(&original, false, false) == 0);

    CHECK(logic_engine_compile("A|B|C|D|E|F", &program, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_engine_truth_row_count(&program) == 64);
    CHECK(!logic_engine_evaluate(&program, 0));
    CHECK(logic_engine_evaluate(&program, 0x3f));

    CHECK(logic_engine_compile("A ^ B", &program, &error) ==
          LOGIC_STATUS_OK);
    char form[LOGIC_FORM_CAPACITY];
    CHECK(logic_engine_format_canonical(&program, true, form,
                                        sizeof form) == LOGIC_STATUS_OK);
    CHECK(strcmp(form, "(A&!B)|(!A&B)") == 0);
    CHECK(logic_engine_format_canonical(&program, false, form,
                                        sizeof form) == LOGIC_STATUS_OK);
    CHECK(strcmp(form, "(A|B)&(!A|!B)") == 0);

    logic_program_t simplified;
    CHECK(logic_engine_compile("(A&B)|(A&!B)", &original, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_engine_format_simplified(&original, true, form,
                                         sizeof form) == LOGIC_STATUS_OK);
    CHECK(logic_engine_compile(form, &simplified, &error) == LOGIC_STATUS_OK);
    CHECK(equivalent(&original, &simplified));
    CHECK(strcmp(form, "(A)") == 0);

    CHECK(logic_engine_compile("(A|B)&(A|!B)", &original, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_engine_format_simplified(&original, false, form,
                                         sizeof form) == LOGIC_STATUS_OK);
    CHECK(logic_engine_compile(form, &simplified, &error) == LOGIC_STATUS_OK);
    CHECK(equivalent(&original, &simplified));

    CHECK(logic_engine_compile("A & (B |", &program, &error) ==
          LOGIC_STATUS_SYNTAX);
    CHECK(error > 0);
    CHECK(logic_engine_compile("", &program, &error) == LOGIC_STATUS_EMPTY);

    puts("logic engine tests passed");
    return 0;
}
