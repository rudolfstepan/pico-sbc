#include "calculator_statistics.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_message(char *message, size_t size, const char *text) {
    if (!message || !size) return;
    snprintf(message, size, "%s", text);
}

static expression_editor_t *active_editor(calculator_statistics_t *stats) {
    return stats->active_y ? &stats->y_editor : &stats->x_editor;
}

static void clear_draft(calculator_statistics_t *stats) {
    expression_editor_clear(&stats->x_editor);
    expression_editor_clear(&stats->y_editor);
    stats->active_y = false;
    stats->editing = false;
}

void calculator_statistics_init(calculator_statistics_t *stats) {
    if (!stats) return;
    memset(stats, 0, sizeof *stats);
    statistics_engine_init(&stats->dataset);
    expression_editor_init(&stats->x_editor);
    expression_editor_init(&stats->y_editor);
    stats->view = STATISTICS_VIEW_DATA;
}

static bool parse_number(const expression_editor_t *editor, double *value) {
    if (!editor->length) return false;
    char *end = NULL;
    errno = 0;
    double parsed = strtod(editor->text, &end);
    while (end && isspace((unsigned char)*end)) end++;
    if (!end || end == editor->text || *end || errno == ERANGE ||
        !isfinite(parsed)) {
        return false;
    }
    *value = parsed;
    return true;
}

static void add_row(calculator_statistics_t *stats,
                    char *message, size_t message_size) {
    double x = 0.0;
    double y = 0.0;
    if (!parse_number(&stats->x_editor, &x) ||
        (stats->dataset.two_variable &&
         !parse_number(&stats->y_editor, &y))) {
        set_message(message, message_size,
                    stats->dataset.two_variable ? "ENTER X AND Y" : "ENTER X");
        return;
    }
    bool updating = stats->editing &&
                    stats->editing_index < stats->dataset.count;
    statistics_status_t status = STATISTICS_STATUS_OK;
    if (updating) {
        stats->dataset.x[stats->editing_index] = x;
        stats->dataset.y[stats->editing_index] = y;
        stats->selected = stats->editing_index;
        set_message(message, message_size, "ROW UPDATED");
    } else {
        stats->editing = false;
        status = statistics_engine_add(&stats->dataset, x, y);
        set_message(message, message_size, statistics_engine_status_text(status));
    }
    if (status == STATISTICS_STATUS_OK) {
        if (!updating) stats->selected = stats->dataset.count - 1;
        clear_draft(stats);
    }
}

static void set_mode(calculator_statistics_t *stats, bool two_variable,
                     char *message, size_t message_size) {
    bool changed = stats->dataset.two_variable != two_variable;
    statistics_engine_set_two_variable(&stats->dataset, two_variable);
    if (changed) clear_draft(stats);
    stats->selected = 0;
    stats->active_y = false;
    stats->view = STATISTICS_VIEW_DATA;
    set_message(message, message_size,
                changed ? "DATA CLEARED" :
                (two_variable ? "2VAR MODE" : "1VAR MODE"));
}

static void select_row(calculator_statistics_t *stats, int direction,
                       char *message, size_t message_size) {
    if (!stats->dataset.count) {
        set_message(message, message_size, "NO DATA");
        return;
    }
    if (stats->editing) clear_draft(stats);
    if (direction < 0) {
        stats->selected = stats->selected ? stats->selected - 1
                                          : stats->dataset.count - 1;
    } else {
        stats->selected = stats->selected + 1 < stats->dataset.count
            ? stats->selected + 1 : 0;
    }
    set_message(message, message_size, "ROW SELECTED");
}

static void edit_row(calculator_statistics_t *stats,
                     char *message, size_t message_size) {
    if (!stats->dataset.count) {
        set_message(message, message_size, "NO DATA");
        return;
    }
    char text[32];
    snprintf(text, sizeof text, "%.17g", stats->dataset.x[stats->selected]);
    expression_editor_set(&stats->x_editor, text);
    if (stats->dataset.two_variable) {
        snprintf(text, sizeof text, "%.17g", stats->dataset.y[stats->selected]);
        expression_editor_set(&stats->y_editor, text);
    }
    stats->editing = true;
    stats->editing_index = stats->selected;
    stats->view = STATISTICS_VIEW_DATA;
    set_message(message, message_size, "ROW LOADED");
}

static void remove_row(calculator_statistics_t *stats,
                       char *message, size_t message_size) {
    statistics_status_t status = statistics_engine_remove(&stats->dataset,
                                                           stats->selected);
    if (status == STATISTICS_STATUS_OK && stats->selected >= stats->dataset.count &&
        stats->selected) {
        stats->selected--;
    }
    if (status == STATISTICS_STATUS_OK) clear_draft(stats);
    set_message(message, message_size, statistics_engine_status_text(status));
}

static void toggle_sign(expression_editor_t *editor) {
    char text[EXPRESSION_EDITOR_CAPACITY];
    if (editor->text[0] == '-') {
        snprintf(text, sizeof text, "%s", editor->text + 1);
    } else {
        if (editor->length + 1 >= sizeof text) return;
        text[0] = '-';
        memcpy(text + 1, editor->text, editor->length + 1);
    }
    expression_editor_set(editor, text);
}

void calculator_statistics_activate(calculator_statistics_t *stats,
                                    const char *token, double ans,
                                    char *message, size_t message_size) {
    if (!stats || !token) return;
    if (strcmp(token, "1VAR") == 0 || strcmp(token, "2VAR") == 0) {
        set_mode(stats, token[0] == '2', message, message_size);
    } else if (strcmp(token, "DATA") == 0) {
        stats->view = STATISTICS_VIEW_DATA;
        set_message(message, message_size, "DATA VIEW");
    } else if (strcmp(token, "SUMMARY") == 0) {
        stats->view = STATISTICS_VIEW_SUMMARY;
        set_message(message, message_size, "SUMMARY");
    } else if (strcmp(token, "REGRESSION") == 0) {
        stats->view = STATISTICS_VIEW_REGRESSION;
        set_message(message, message_size,
                    stats->dataset.two_variable ? "REGRESSION" :
                                                  "2VAR REQUIRED");
    } else if (strcmp(token, "PLOT") == 0) {
        stats->view = STATISTICS_VIEW_PLOT;
        set_message(message, message_size,
                    stats->dataset.two_variable ? "SCATTER PLOT" :
                                                  "HISTOGRAM");
    } else if (strcmp(token, "XY") == 0) {
        if (stats->dataset.two_variable) stats->active_y = !stats->active_y;
        set_message(message, message_size,
                    stats->active_y ? "Y FIELD" : "X FIELD");
    } else if (strcmp(token, "DEL") == 0) {
        expression_editor_delete(active_editor(stats));
        stats->view = STATISTICS_VIEW_DATA;
        set_message(message, message_size, "EDIT");
    } else if (strcmp(token, "ADD") == 0) {
        stats->view = STATISTICS_VIEW_DATA;
        add_row(stats, message, message_size);
    } else if (strcmp(token, "ANS") == 0) {
        char text[32];
        snprintf(text, sizeof text, "%.17g", ans);
        expression_editor_set(active_editor(stats), text);
        stats->view = STATISTICS_VIEW_DATA;
        set_message(message, message_size, "ANS LOADED");
    } else if (strcmp(token, "PREV") == 0 || strcmp(token, "NEXT") == 0) {
        select_row(stats, token[0] == 'P' ? -1 : 1,
                   message, message_size);
    } else if (strcmp(token, "EDIT") == 0) {
        edit_row(stats, message, message_size);
    } else if (strcmp(token, "DROP") == 0) {
        remove_row(stats, message, message_size);
    } else if (strcmp(token, "CLEAR") == 0) {
        statistics_engine_clear(&stats->dataset);
        clear_draft(stats);
        stats->selected = 0;
        stats->view = STATISTICS_VIEW_DATA;
        set_message(message, message_size, "DATA CLEARED");
    } else if (strcmp(token, "SIGN") == 0) {
        toggle_sign(active_editor(stats));
        stats->view = STATISTICS_VIEW_DATA;
        set_message(message, message_size, "EDIT");
    } else {
        stats->view = STATISTICS_VIEW_DATA;
        if (expression_editor_insert(active_editor(stats), token)) {
            set_message(message, message_size, "EDIT");
        } else {
            set_message(message, message_size, "INPUT FULL");
        }
    }
}
