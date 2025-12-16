#ifndef ARTICLE_H
#define ARTICLE_H

#include <stddef.h>
#include "differentiator.h"

bool latex_too_long(const char *latex);
void log_placeholder(FILE *article_file, const char *message, size_t len);
void differentiate_set_article_file(FILE *file);
void differentiate_set_article_tree(const FRONT_COMPIL_T *tree);
FILE *differentiate_get_article_stream(void);
const FRONT_COMPIL_T *differentiate_get_article_tree(void);
void article_log_text(const char *fmt, ...);
void article_log_with_latex(const FRONT_COMPIL_T *tree, const char *fmt, ...);
void article_log_transition(const char *phrase, char *before_latex, char *after_latex);
const char *article_phrase(const NODE_T *node);
void article_log_step(const NODE_T *node, NODE_T *result);

extern size_t g_step_counter;
extern size_t g_step_limit;
extern size_t g_requested_step_limit;

#endif