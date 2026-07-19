#ifndef EXPRESSION_EDITOR_H
#define EXPRESSION_EDITOR_H

#include <stdbool.h>
#include <stddef.h>

#define EXPRESSION_EDITOR_CAPACITY 96

typedef enum {
    EDITOR_CURSOR_LEFT,
    EDITOR_CURSOR_RIGHT,
    EDITOR_CURSOR_HOME,
    EDITOR_CURSOR_END
} editor_cursor_action_t;

typedef struct {
    char text[EXPRESSION_EDITOR_CAPACITY];
    size_t length;
    size_t cursor;
} expression_editor_t;

void expression_editor_init(expression_editor_t *editor);
void expression_editor_clear(expression_editor_t *editor);
bool expression_editor_set(expression_editor_t *editor, const char *text);
bool expression_editor_insert(expression_editor_t *editor, const char *token);
bool expression_editor_delete(expression_editor_t *editor);
void expression_editor_move(expression_editor_t *editor,
                            editor_cursor_action_t action);
const char *expression_editor_view(const expression_editor_t *editor,
                                   char *buffer, size_t buffer_size,
                                   size_t max_chars);

#endif
