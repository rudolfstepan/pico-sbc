#include "expression_editor.h"

#include <stdio.h>
#include <string.h>

void expression_editor_init(expression_editor_t *editor) {
    expression_editor_clear(editor);
}

void expression_editor_clear(expression_editor_t *editor) {
    editor->text[0] = '\0';
    editor->length = 0;
    editor->cursor = 0;
}

bool expression_editor_set(expression_editor_t *editor, const char *text) {
    size_t length = strlen(text);
    if (length >= sizeof editor->text) return false;

    memcpy(editor->text, text, length + 1);
    editor->length = length;
    editor->cursor = length;
    return true;
}

bool expression_editor_insert(expression_editor_t *editor, const char *token) {
    size_t token_length = strlen(token);
    if (!token_length) return true;
    if (editor->length + token_length >= sizeof editor->text) return false;

    memmove(editor->text + editor->cursor + token_length,
            editor->text + editor->cursor,
            editor->length - editor->cursor + 1);
    memcpy(editor->text + editor->cursor, token, token_length);
    editor->length += token_length;
    editor->cursor += token_length;
    return true;
}

bool expression_editor_delete(expression_editor_t *editor) {
    if (!editor->cursor) return false;

    memmove(editor->text + editor->cursor - 1,
            editor->text + editor->cursor,
            editor->length - editor->cursor + 1);
    editor->cursor--;
    editor->length--;
    return true;
}

void expression_editor_move(expression_editor_t *editor,
                            editor_cursor_action_t action) {
    switch (action) {
        case EDITOR_CURSOR_LEFT:
            if (editor->cursor) editor->cursor--;
            break;
        case EDITOR_CURSOR_RIGHT:
            if (editor->cursor < editor->length) editor->cursor++;
            break;
        case EDITOR_CURSOR_HOME:
            editor->cursor = 0;
            break;
        case EDITOR_CURSOR_END:
            editor->cursor = editor->length;
            break;
    }
}

const char *expression_editor_view(const expression_editor_t *editor,
                                   char *buffer, size_t buffer_size,
                                   size_t max_chars) {
    if (!buffer_size) return buffer;
    if (editor->length + 2 > buffer_size) {
        snprintf(buffer, buffer_size, "_");
        return buffer;
    }

    memcpy(buffer, editor->text, editor->cursor);
    buffer[editor->cursor] = '_';
    memcpy(buffer + editor->cursor + 1,
           editor->text + editor->cursor,
           editor->length - editor->cursor + 1);

    size_t view_length = editor->length + 1;
    size_t start = 0;
    if (view_length > max_chars) {
        size_t half = max_chars / 2;
        start = editor->cursor > half ? editor->cursor - half : 0;
        if (start + max_chars > view_length) start = view_length - max_chars;
    }
    return buffer + start;
}
