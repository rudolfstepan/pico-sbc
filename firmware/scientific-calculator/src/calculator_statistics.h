#ifndef CALCULATOR_STATISTICS_H
#define CALCULATOR_STATISTICS_H

#include "expression_editor.h"
#include "statistics_engine.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    STATISTICS_VIEW_DATA,
    STATISTICS_VIEW_SUMMARY,
    STATISTICS_VIEW_REGRESSION,
    STATISTICS_VIEW_PLOT
} calculator_statistics_view_t;

typedef struct {
    statistics_dataset_t dataset;
    expression_editor_t x_editor;
    expression_editor_t y_editor;
    calculator_statistics_view_t view;
    size_t selected;
    size_t editing_index;
    bool active_y;
    bool editing;
} calculator_statistics_t;

void calculator_statistics_init(calculator_statistics_t *statistics);
void calculator_statistics_activate(calculator_statistics_t *statistics,
                                    const char *token, double ans,
                                    char *message, size_t message_size);

#endif
