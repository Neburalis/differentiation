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
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(prompt_buf, sizeof(prompt_buf), fmt, ap);
    va_end(ap);

    FILE *article_file = differentiate_get_article_stream();
    if (!article_file || !fmt || !*fmt) return;
    fprintf(article_file, "%s\n\n", prompt_buf);
    fflush(article_file);
}

void article_log_with_latex(const EQ_TREE_T *tree, const char *fmt, ...) {
    char prompt_buf[2048] = "";
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(prompt_buf, sizeof(prompt_buf), fmt, ap);
    va_end(ap);

    FILE *article_file = differentiate_get_article_stream();
    if (!article_file || !tree || !tree->root) return;

    EQ_TREE_T *mutable_tree = (EQ_TREE_T *) tree;
    char *latex = latex_dump(mutable_tree);
    if (!latex) return;

    if (fmt && *fmt) fprintf(article_file, "%s\n\n", prompt_buf);
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

function const char *article_phrase(const NODE_T *node) {
    if (!node || node->type != OP_T) return nullptr;
    return get_random_str_to_dif_op(node->value.opr);
}

function void article_log_step(const NODE_T *node, NODE_T *result) {
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

    const char *phrase = article_phrase(node);
    if (phrase && phrase[0] != '\0')
        fprintf(article_file, "%s\n\n", phrase);

    if (source_latex && result_latex)
        fprintf(article_file, "\\begin{dmath*}\n(%s)' = %s\n\\end{dmath*}\n\n", source_latex, result_latex);
    else if (source_latex)
        fprintf(article_file, "\\begin{dmath*}\n(%s)' = ?\n\\end{dmath*}\n\n", source_latex);

    if (source_latex) FREE(source_latex);
    if (result_latex) FREE(result_latex);

    fflush(article_file);
}

function NODE_T *copy_subtree(const NODE_T *node) {
    if (!node) return nullptr;
    NODE_T *left = node->left ? copy_subtree(node->left) : nullptr;
    if (node->left && !left) return nullptr;
    NODE_T *right = node->right ? copy_subtree(node->right) : nullptr;
    if (node->right && !right) {
        destruct(left);
        return nullptr;
    }
    NODE_T *copy = new_node(node->type, node->value, left, right);
    if (!copy) {
        destruct(left);
        destruct(right);
    }
    if (copy) {
        if (copy->left ) copy->left ->parent = copy;
        if (copy->right) copy->right->parent = copy;
    }
    return copy;
}

function NODE_T *make_number(double value) {
    return new_node(NUM_T, (NODE_VALUE_T) {.num = value}, nullptr, nullptr);
}

function NODE_T *make_binary(OPERATOR op, NODE_T *left, NODE_T *right) {
    NODE_T *node = new_node(OP_T, (NODE_VALUE_T) {.opr = op}, left, right);
    if (!node) {
        destruct(left);
        destruct(right);
    }
    if (node) {
        if (node->left ) node->left ->parent = node;
        if (node->right) node->right->parent = node;
    }
    return node;
}

function NODE_T *make_unary(OPERATOR op, NODE_T *arg) {
    NODE_T *node = new_node(OP_T, (NODE_VALUE_T) {.opr = op}, arg, nullptr);
    if (!node) { destruct(node); return nullptr; }
    if (node && node->left) node->left->parent = node;
    return node;
}

function NODE_T *make_variable(size_t var_idx) {
    NODE_T *node = new_node(VAR_T, (NODE_VALUE_T) {.var = var_idx}, nullptr, nullptr);
    if (!node) { destruct(node); return nullptr; }
    return node;
}

#define ADD(L, R) make_binary(ADD, (L), (R))
#define SUB(L, R) make_binary(SUB, (L), (R))
#define MUL(L, R) make_binary(MUL, (L), (R))
#define DIV(L, R) make_binary(DIV, (L), (R))
#define POW(L, R) make_binary(POW, (L), (R))
#define NUM(V)    make_number((V))
#define LN(X)     make_unary(LN, (X))
#define SIN(X)    make_unary(SIN, (X))
#define COS(X)    make_unary(COS, (X))
#define TAN(X)    make_unary(TAN, (X))
#define CTG(X)    make_unary(CTG, (X))
#define ASIN(X)   make_unary(ASIN, (X))
#define ACOS(X)   make_unary(ACOS, (X))
#define ATAN(X)   make_unary(ATAN, (X))
#define ACTG(X)   make_unary(ACTG, (X))
#define SQRT(X)   make_unary(SQRT, (X))
#define SINH(X)   make_unary(SINH, (X))
#define COSH(X)   make_unary(COSH, (X))
#define TANH(X)   make_unary(TANH, (X))
#define CTH(X)    make_unary(CTH, (X))

#define ZERO() make_number(0.0)
#define ONE()  make_number(1.0)

#define dl differentiate_node(node->left, diff_var_idx)
#define dr differentiate_node(node->right, diff_var_idx)
#define cl copy_subtree(node->left)
#define cr copy_subtree(node->right)

#define RES(x) {result = (x); break;}

#define POW_FULL {                                                                      \
        bool base_const = !subtree_contains_var(node->left, diff_var_idx);              \
        bool exp_const  = !subtree_contains_var(node->right, diff_var_idx);             \
        if (base_const && exp_const)  RES(ZERO());                                      \
        if (base_const && !exp_const) RES(MUL(POW(cl, cr), MUL(LN(cl), dr)));           \
        if (!base_const && exp_const && node->right && node->right->type == NUM_T) {    \
            double n = node->right->value.num;                                          \
            RES(MUL(NUM(n), MUL(POW(cl, NUM(n - 1.0)), dl)));                           \
        }                                                                               \
        RES(MUL(POW(cl, cr), ADD(MUL(dr, LN(cl)), DIV(MUL(cr, dl), cl))));              \
    }

#define LOG_FULL {                                                                                          \
    bool arg_const  = !subtree_contains_var(node->left,  diff_var_idx);                                     \
    bool base_const = !subtree_contains_var(node->right, diff_var_idx);                                     \
    if (arg_const  &&  base_const) RES(ZERO());                                                             \
    if (base_const && !arg_const)  RES(DIV(dl, MUL(LN(cr), cl)));                                           \
    if (arg_const  && !base_const) RES(MUL(NUM(-1.0), DIV(MUL(LN(cl), dr), MUL(cr, MUL(LN(cr), LN(cr)))))); \
    RES(DIV(SUB(MUL(DIV(dl, cl), LN(cr)), MUL(LN(cl), DIV(dr, cr))), MUL(LN(cr), LN(cr))));                 \
}

function bool subtree_contains_var(const NODE_T *node, size_t diff_var_idx) {
    if (!node || diff_var_idx == varlist::NPOS) return false;
    if (node->type == VAR_T) return node->value.var == diff_var_idx;
    if (node->type != OP_T) return false;
    return subtree_contains_var(node->left, diff_var_idx) || subtree_contains_var(node->right, diff_var_idx);
}

function NODE_T *differentiate_node(const NODE_T *node, size_t diff_var_idx) {
    if (!node) return nullptr;
    NODE_T *result = nullptr;
    switch (node->type) {
        case NUM_T: RES(ZERO());
        case VAR_T: RES((diff_var_idx != varlist::NPOS && node->value.var == diff_var_idx) ? ONE() : ZERO());
        case OP_T: {
            switch (node->value.opr) {
                case ADD:  RES(ADD(dl, dr));
                case SUB:  RES(SUB(dl, dr));
                case MUL:  RES(ADD(MUL(dl, cr), MUL(cl, dr)));
                case DIV:  RES(DIV(SUB(MUL(dl, cr), MUL(cl, dr)), MUL(cr, cr)));
                case POW:  POW_FULL;
                case LOG:  LOG_FULL;
                case LN:   RES(DIV(dl, cl));
                case SIN:  RES(MUL(COS(cl), dl));
                case COS:  RES(MUL(NUM(-1.0), MUL(SIN(cl), dl)));
                case TAN:  RES(DIV(dl, MUL(COS(cl), COS(cl))));
                case CTG:  RES(MUL(NUM(-1.0), DIV(dl, MUL(SIN(cl), SIN(cl)))));
                case ASIN: RES(DIV(dl, SQRT(SUB(ONE(), MUL(cl, cl)))));
                case ACOS: RES(MUL(NUM(-1.0), DIV(dl, SQRT(SUB(ONE(), MUL(cl, cl))))));
                case ATAN: RES(DIV(dl, ADD(ONE(), MUL(cl, cl))));
                case ACTG: RES(MUL(NUM(-1.0), DIV(dl, ADD(ONE(), MUL(cl, cl)))));
                case SQRT: RES(DIV(dl, MUL(NUM(2.0), SQRT(cl))));
                case SINH: RES(MUL(COSH(cl), dl));
                case COSH: RES(MUL(SINH(cl), dl));
                case TANH: RES(DIV(dl, MUL(COSH(cl), COSH(cl))));
                case CTH:  RES(MUL(NUM(-1.0), DIV(dl, MUL(SINH(cl), SINH(cl)))));
                default:   RES(nullptr);
            }
            break;
        }
        default: return nullptr;
    }
    if (!result) return nullptr;
    article_log_step(node, result);
    return result;
}

function char *get_new_name(const EQ_TREE_T *src, size_t diff_var_idx) {
    const char *original_name = (src->name && *src->name) ? src->name : "equation";
    const mystr::mystr_t *var_entry = (src->vars && diff_var_idx < varlist::size(src->vars))
                                      ? varlist::get(src->vars, diff_var_idx)
                                      : nullptr;
    const char *var_name = (var_entry && var_entry->str && *var_entry->str)
                           ? var_entry->str
                           : "unknown variable";
    const char *prefix = "derivative of ";
    const char *infix = " with respect to ";
    size_t label_len = strlen(prefix)
                     + strlen(original_name)
                     + strlen(infix)
                     + strlen(var_name);
    char *label = TYPED_CALLOC(label_len + 1, char);
    if (!label) {
        return nullptr;
    }
    snprintf(label, label_len + 1, "%s%s%s%s", prefix, original_name, infix, var_name);
    return label;
}

EQ_TREE_T *differentiate(const EQ_TREE_T *src, size_t diff_var_idx) {
    if (!src || !src->root) return nullptr;
    const EQ_TREE_T *prev_tree = differentiate_get_article_tree();
    differentiate_set_article_tree(src);
    FILE *article_file = differentiate_get_article_stream();
    article_log_text("\\bigskip\\hrule\\bigskip\n\\section*{Посчитаем производную}");

    char *origin_latex = latex_dump((EQ_TREE_T *) src);
    article_log_text("Исходное выражение: \n\\begin{dmath*}f(x) = %s\\end{dmath*}", origin_latex);
    FREE(origin_latex);

    article_log_text("Продифференцируем это чудо...\n\n");

    NODE_T *root = differentiate_node(src->root, diff_var_idx);
    differentiate_set_article_tree(prev_tree);
    if (!root) return nullptr;
    root->parent = nullptr;
    varlist::VarList *vars_copy = src->vars ? varlist::clone(src->vars) : nullptr;

    VERIFY(!(src->vars && !vars_copy), destruct(root); return nullptr;)
    EQ_TREE_T *res = TYPED_CALLOC(1, EQ_TREE_T);
    VERIFY(res, destruct(root); return nullptr;);

    res->name = get_new_name(src, diff_var_idx);
    res->root = root;
    res->vars = vars_copy;
    res->owns_vars = vars_copy != nullptr;
    res->owns_name = true;
    article_log_with_latex(res, "Получили производную. Теперь упростим это выражение:");
    simplify_tree(res);
    return res;
}

// Вернет указатель на динамический массив деревьев, где на i-том индексе лежит i-тая производная выражения
// (на 0 лежит само исходное выражение)
// на n+1 индексе лежит nullptr как терминальный элемент конца массива
EQ_TREE_T **differentiate_to_n(const EQ_TREE_T *src, size_t n, size_t diff_var_idx) {
    EQ_TREE_T **array = TYPED_CALLOC(n + 2, EQ_TREE_T *);
    array[0] = (EQ_TREE_T *) src;

    FILE *prev_article_file = differentiate_get_article_stream();
    differentiate_set_article_file(nullptr);
    for (size_t i = 1; i <= n; ++i) {
        // printf("i = %zu", i);
        EQ_TREE_T *dif = differentiate(array[i-1], diff_var_idx);
        if (dif == nullptr) {
            ERROR_MSG("Не смог взять %zu-тую производную\n", i);
            destruct(array);
            FREE(array);
            return nullptr;
        }
        array[i] = dif;
    }
    differentiate_set_article_file(prev_article_file);
    return array;
}

function NODE_T *tailor_k_term(EQ_TREE_T *k_dif, size_t k, double point, size_t var_idx) {
    double koef = 0;
    double point_values[1] = {point};
    EQ_POINT_T calc_point = {.tree = k_dif, .point = point_values, .vars_count = 1};
    calc_in_point(&calc_point);
    koef = calc_point.result / tgamma(k + 1); // gamma(x) = (x - 1)!
    return MUL(make_number(koef), POW(SUB(make_variable(var_idx), make_number(point)), make_number(k)));
}

function NODE_T *build_taylor_expression(EQ_TREE_T **diff_array, size_t n, double point, size_t var_idx) {
    NODE_T *expr = nullptr;
    for (size_t k = 0; k <= n; ++k) {
        NODE_T *term = tailor_k_term(diff_array[k], k, point, var_idx);
        if (!term) {
            destruct(expr);
            return nullptr;
        }
        if (!expr) {
            expr = term;
            expr->parent = nullptr;
            continue;
        }
        NODE_T *sum = make_binary(ADD, expr, term);
        if (!sum) {
            destruct(term);
            destruct(expr);
            return nullptr;
        }
        expr->parent = sum;
        term->parent = sum;
        expr = sum;
    }
    return expr;
}

EQ_TREE_T *tailor_formula(EQ_TREE_T **diff_array, size_t n, double point, size_t var_idx) {
    NODE_T *root = build_taylor_expression(diff_array, n, point, var_idx);
    if (!root) return nullptr;

    EQ_TREE_T *tailor_tree = TYPED_CALLOC(1, EQ_TREE_T);
    if (tailor_tree == nullptr) {
        destruct(root);
        return nullptr;
    }
    
    tailor_tree->root = root;
    tailor_tree->name = strdup(diff_array[0]->name);
    tailor_tree->vars = varlist::clone(diff_array[0]->vars);
    tailor_tree->owns_name = true;
    tailor_tree->owns_vars = true;

    simplify_tree(tailor_tree);

    return tailor_tree;
}

#undef dl
#undef dr
#undef cl
#undef cr

#undef ADD
#undef SUB
#undef MUL
#undef DIV
#undef POW
#undef NUM
#undef LN
#undef SIN
#undef COS
#undef TAN
#undef CTG
#undef ASIN
#undef ACOS
#undef ATAN
#undef ZERO
#undef ONE

#undef RES
#undef LOG_FULL
#undef POW_FULL