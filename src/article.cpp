#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "differentiator.h"
#include "base.h"
#include "io_utils.h"
#include "logger.h"

extern const char *get_random_str_to_dif_op(OPERATOR op);

typedef struct {
    FILE            *file;
    const EQ_TREE_T *tree;
} ArticleContext;

global ArticleContext ARTICLE_CONTEXT = {};

size_t g_step_counter = 0;
size_t g_step_limit = 0;
size_t g_requested_step_limit = 0;

bool latex_too_long(const char *latex) {
    // if (!latex) return false;
    // size_t len = strlen(latex);
    // if (len > 900) return true;
    // size_t cdot_count = 0;
    // for (const char *p = latex; *p; ++p) {
    //     if (p[0] == '\\' && strncmp(p, "\\cdot", 5) == 0) {
    //         cdot_count++;
    //     }
    // }
    // return cdot_count > 80;
    return false;
}

void log_placeholder(FILE *article_file, const char *message, size_t len) {
    if (!article_file) return;
    if (len)
        fprintf(article_file, "\\textit{%s (%zu символов).}\n\n", message, len);
    else
        fprintf(article_file, "\\textit{%s.}\n\n", message);
}

void differentiate_set_article_file(FILE *file) {
    ARTICLE_CONTEXT.file = file;
}

void differentiate_set_article_tree(const EQ_TREE_T *tree) {
    ARTICLE_CONTEXT.tree = tree;
}

FILE *differentiate_get_article_stream(void) {
    return ARTICLE_CONTEXT.file ? ARTICLE_CONTEXT.file : logger_get_file();
}

const EQ_TREE_T *differentiate_get_article_tree(void) {
    return ARTICLE_CONTEXT.tree;
}

void article_log_text(const char *fmt, ...) {
    char prompt_buf[32768] = "";
    if (fmt && *fmt) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(prompt_buf, sizeof(prompt_buf), fmt, ap);
        va_end(ap);
    }

    FILE *article_file = differentiate_get_article_stream();
    if (!article_file || !fmt || !*fmt) return;
    fprintf(article_file, "%s\n\n", prompt_buf);
    fflush(article_file);
}

void article_log_with_latex(const EQ_TREE_T *tree, const char *fmt, ...) {
    char prompt_buf[2048] = "";
    if (fmt && *fmt) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(prompt_buf, sizeof(prompt_buf), fmt, ap);
        va_end(ap);
    }

    FILE *article_file = differentiate_get_article_stream();
    if (!article_file || !tree || !tree->root) return;

    EQ_TREE_T *mutable_tree = (EQ_TREE_T *) tree;
    char *latex = latex_dump(mutable_tree);
    if (!latex) return;

    if (fmt && *fmt) fprintf(article_file, "%s\n\n", prompt_buf);
    if (latex_too_long(latex)) {
        log_placeholder(article_file, "Формула опущена: слишком длинное выражение", strlen(latex));
        FREE(latex);
        fflush(article_file);
        return;
    }
    fprintf(article_file, "\\begin{dmath*}\n%s\n\\end{dmath*}\n\n", latex);
    FREE(latex);
    fflush(article_file);
}

void article_log_transition(const char *phrase, char *before_latex, char *after_latex) {
    FILE *article_file = differentiate_get_article_stream();
    if (!article_file) {
        if (before_latex) FREE(before_latex);
        if (after_latex) FREE(after_latex);
        return;
    }
    if (phrase && *phrase) fprintf(article_file, "%s\n\n", phrase);

    if (before_latex && after_latex)
        fprintf(article_file, "\\begin{dmath*}\n%s \\Rightarrow %s\n\\end{dmath*}\n\n", before_latex, after_latex);
    else if (after_latex)
        fprintf(article_file, "\\begin{dmath*}\n%s\n\\end{dmath*}\n\n", after_latex);
    else if (before_latex)
        fprintf(article_file, "\\begin{dmath*}\n%s\n\\end{dmath*}\n\n", before_latex);

    if (before_latex) FREE(before_latex);
    if (after_latex) FREE(after_latex);
    fflush(article_file);
}

const char *article_phrase(const NODE_T *node) {
    if (!node || node->type != OP_T) return nullptr;
    return get_random_str_to_dif_op(node->value.opr);
}

void article_log_step(const NODE_T *node, NODE_T *result) {
    if (!node || !result) return;

    FILE *article_file = differentiate_get_article_stream();
    const EQ_TREE_T *current_tree = differentiate_get_article_tree();
    if (!article_file || !current_tree) return;

    EQ_TREE_T source_eq = {};
    source_eq.root = (NODE_T *) node;
    source_eq.vars = current_tree->vars;

    EQ_TREE_T result_eq = {};
    result_eq.root = result;
    result_eq.vars = current_tree->vars;

    char *source_latex = latex_dump(&source_eq);
    char *result_latex = latex_dump(&result_eq);

    if (g_step_limit && g_step_counter >= g_step_limit) {
        if (g_step_counter == g_step_limit) {
            log_placeholder(article_file, "Оставшиеся шаги дифференцирования опущены", 0);
        }
        if (source_latex) FREE(source_latex);
        if (result_latex) FREE(result_latex);
        g_step_counter++;
        return;
    }

    size_t src_len = source_latex ? strlen(source_latex) : 0;
    size_t res_len = result_latex ? strlen(result_latex) : 0;
    if (latex_too_long(source_latex) || latex_too_long(result_latex)) {
        log_placeholder(article_file, "Шаг пропущен: громоздкое выражение", src_len + res_len);
        if (source_latex) FREE(source_latex);
        if (result_latex) FREE(result_latex);
        g_step_counter++;
        return;
    }

    const char *phrase = article_phrase(node);
    if (phrase && phrase[0] != '\0')
        fprintf(article_file, "%s\n\n", phrase);

    if (source_latex && result_latex)
        fprintf(article_file, "\\begin{dmath*}\n(%s)' = %s\n\\end{dmath*}\n\n", source_latex, result_latex);
    else if (source_latex)
        fprintf(article_file, "\\begin{dmath*}\n(%s)' = ?\n\\end{dmath*}\n\n", source_latex);

    if (source_latex) FREE(source_latex);
    if (result_latex) FREE(result_latex);

    g_step_counter++;

    fflush(article_file);
}