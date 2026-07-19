#include "expression_editor.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    expression_editor_t editor;
    char view[EXPRESSION_EDITOR_CAPACITY + 2];

    expression_editor_init(&editor);
    CHECK(editor.length == 0);
    CHECK(editor.cursor == 0);
    CHECK(strcmp(expression_editor_view(&editor, view, sizeof view, 10),
                 "_") == 0);

    CHECK(expression_editor_insert(&editor, "12"));
    CHECK(expression_editor_insert(&editor, "+34"));
    CHECK(strcmp(editor.text, "12+34") == 0);
    expression_editor_move(&editor, EDITOR_CURSOR_LEFT);
    expression_editor_move(&editor, EDITOR_CURSOR_LEFT);
    CHECK(expression_editor_insert(&editor, "*"));
    CHECK(strcmp(editor.text, "12+*34") == 0);
    CHECK(strcmp(expression_editor_view(&editor, view, sizeof view, 20),
                 "12+*_34") == 0);
    CHECK(expression_editor_delete(&editor));
    CHECK(strcmp(editor.text, "12+34") == 0);

    expression_editor_move(&editor, EDITOR_CURSOR_HOME);
    CHECK(!expression_editor_delete(&editor));
    expression_editor_move(&editor, EDITOR_CURSOR_RIGHT);
    expression_editor_move(&editor, EDITOR_CURSOR_END);
    CHECK(editor.cursor == editor.length);

    CHECK(expression_editor_set(&editor, "1234567890"));
    expression_editor_move(&editor, EDITOR_CURSOR_LEFT);
    expression_editor_move(&editor, EDITOR_CURSOR_LEFT);
    CHECK(strcmp(expression_editor_view(&editor, view, sizeof view, 6),
                 "678_90") == 0);

    char full[EXPRESSION_EDITOR_CAPACITY];
    memset(full, '1', sizeof full - 1);
    full[sizeof full - 1] = '\0';
    CHECK(expression_editor_set(&editor, full));
    CHECK(!expression_editor_insert(&editor, "2"));

    expression_editor_clear(&editor);
    CHECK(editor.text[0] == '\0');
    puts("expression editor tests passed");
    return 0;
}
