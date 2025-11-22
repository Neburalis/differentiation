#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "differentiator.h"
#include "enhanced_string.h"
#include "stringNthong.h"
#include "io_utils.h"
#include "base.h"
#include "var_list.h"

NODE_T *alloc_new_node() {
    NODE_T *new_node = TYPED_CALLOC(1, NODE_T);
    if (new_node == nullptr) {
        return nullptr;
    }
    new_node->signature=signature;
    return new_node;
}

NODE_T *new_node(NODE_TYPE type, NODE_VALUE_T value, NODE_T *left, NODE_T *right) {
    NODE_T *node = alloc_new_node();
    if (!node)
        return nullptr;
    node->type  = type;
    node->value = value;
    node->left  = left;
    node->right = right;
    if (right) {
        right->parent = node;
        node->elements += right->elements + 1;
    }
    if (left) {
        left->parent = node;
        node->elements += left->elements + 1;
    }
    return node;
}

void destruct(NODE_T *node) {
    if (!node)
        return;
    destruct(node->left);
    destruct(node->right);
    FREE(node);
}

void destruct(EQ_TREE_T *eqtree) {
    if (!eqtree)
        return;
    destruct(eqtree->root);
    varlist::destruct(eqtree->vars);
    FREE(eqtree);
}

bool node_type_from_token(const char *token, NODE_TYPE *out) {
    if (!token || !*token || !out)
        return false;
    if (!token[1] && strchr("+-*/", token[0])) {
        *out = OP_T;
        return true;
    }
    OPERATOR tmp = {};
    if (operator_from_token(token, &tmp)) {
        *out = OP_T;
        return true;
    }
    char *end = nullptr;
    strtod(token, &end);
    if (end && end != token && *end == '\0') {
        *out = NUM_T;
        return true;
    }
    if (isalpha((unsigned char)token[0])) {
        for (const unsigned char *c = (const unsigned char *)token; *c; ++c) {
            if (!isalnum(*c) && *c != '_')
                return false;
        }
        *out = VAR_T;
        return true;
    }
    return false;
}

bool operator_from_token(const char *token, OPERATOR *op) {
    if (!token || !op || !*token)
        return false;
#define MATCH(tok, val) if (strcmp(token, (tok)) == 0) { *(op) = (val); return true; }
    MATCH("+", ADD);
    MATCH("-", SUB);
    MATCH("*", MUL);
    MATCH("/", DIV);
    MATCH("^", POW);
    MATCH("ln", LN);
    MATCH("log", LOG);
    MATCH("sin", SIN);
    MATCH("cos", COS);
    MATCH("tan", TAN);
    MATCH("tg", TAN);
    MATCH("cot", CTG);
    MATCH("ctg", CTG);
    MATCH("arcsin", ASIN);
    MATCH("arccos", ACOS);
    MATCH("arctan", ATAN);
    MATCH("arctg", ATAN);
    MATCH("arccot", ACTG);
    MATCH("arcctg", ACTG);
    MATCH("sqrt", SQRT);
    MATCH("sinh", SINH);
    MATCH("sh", SINH);
    MATCH("cosh", COSH);
    MATCH("ch", COSH);
    MATCH("tanh", TANH);
    MATCH("th", TANH);
    MATCH("coth", CTH);
    MATCH("cth", CTH);
#undef MATCH
    return false;
}

bool is_leaf(const NODE_T *node) {
    return node && node->left && node->right;
}