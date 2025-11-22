#ifndef DIFFERENTIATION_H
#define DIFFERENTIATION_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "var_list.h"

const int32_t signature = (int32_t) 0x50C0C1;

typedef enum {
    NUM_T, OP_T, VAR_T,
} NODE_TYPE;

typedef enum {
    ADD, SUB, MUL, DIV,
    POW,
    LOG, LN,
    SIN, COS, TAN, CTG,
    ASIN, ACOS, ATAN, ACTG,
    SQRT,
    SINH, COSH, TANH, CTH,
} OPERATOR;

typedef union {
    double    num;
    OPERATOR  opr;
    size_t    var;
} NODE_VALUE_T;

typedef struct NODE_T {
    int32_t         signature;

    NODE_TYPE       type;
    NODE_VALUE_T    value;

    size_t          elements;

    NODE_T          *left, *right,
                    *parent;
} NODE_T;

typedef struct EQ_TREE_T {
    const char       *name;
    NODE_T           *root;
    varlist::VarList *vars;
} EQ_TREE_T;

typedef enum {
    TREE_NO_PROBLEM,
    TREE_CANT_ALLOC_MEMORY,
} TREE_ERRNO;

NODE_T *alloc_new_node();

NODE_T *new_node(NODE_TYPE type, NODE_VALUE_T value, NODE_T *left, NODE_T *right);

EQ_TREE_T *load_tree_from_file(const char *filename, varlist::VarList *vars);
EQ_TREE_T *load_tree_from_file(const char *filename, const char *eq_tree_name, varlist::VarList *vars);

void destruct(EQ_TREE_T *eqtree);
void destruct(NODE_T    *node);

bool node_type_from_token(const char *token, NODE_TYPE *out);
const char *node_type_name(const NODE_T *node);

bool operator_from_token(const char *token, OPERATOR *op);
const char *operator_symbol(OPERATOR op);

void format_node_value(const EQ_TREE_T *eqtree, const NODE_T *node, char *buf, size_t size);

typedef struct {
    const EQ_TREE_T *tree;
    double          *point;
    size_t           vars_count;
    double           result;
} EQ_POINT_T;

EQ_POINT_T  read_point_data(const EQ_TREE_T *eqtree);
EQ_POINT_T *calc_in_point  (EQ_POINT_T *point);

void save_tree_to_file(FILE *file, EQ_TREE_T *eqtree);

bool verifier(EQ_TREE_T *eqtree);

void print(EQ_TREE_T *eqtree);

bool is_leaf(const NODE_T *node);

// ---- Dump ----

// typedef struct {
//     NODE_T node;
//     char   *params;
// } DUMP_CONF_T;

// Полный дамп со всеми полями и указателями
void full_dump(EQ_TREE_T *node);
void full_dump(EQ_TREE_T *node, const char *fmt, ...) __attribute__((format (printf, 2, 3)));
// void full_dump(NODE_T node, DUMP_CONF_T *node_confs, char *fmt, ...) __attribute__((format (printf, 3, 4)));

// Дамп только содержимого (если нет никаких проблем с указателями)
void simple_dump(EQ_TREE_T *node);
void simple_dump(EQ_TREE_T *node, const char *fmt, ...) __attribute__((format (printf, 2, 3)));
// void simple_dump(NODE_T node, DUMP_CONF_T *node_confs, char *fmt, ...) __attribute__((format (printf, 3, 4)));

// Возвращает указатель на строку, содержащую latex выражение
char *latex_dump(EQ_TREE_T *node);

#endif //DIFFERENTIATION_H