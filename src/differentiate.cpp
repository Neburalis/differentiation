#include <string.h>
#include "differentiator.h"
#include "base.h"

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
    if (!node) destruct(arg);
    if (node && node->left) node->left->parent = node;
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
#define ONE() make_number(1.0)

#define dl differentiate_node(node->left, diff_var_idx)
#define dr differentiate_node(node->right, diff_var_idx)
#define cl copy_subtree(node->left)
#define cr copy_subtree(node->right)

function bool subtree_contains_var(const NODE_T *node, size_t diff_var_idx) {
    if (!node || diff_var_idx == varlist::NPOS) return false;
    if (node->type == VAR_T) return node->value.var == diff_var_idx;
    if (node->type != OP_T) return false;
    return subtree_contains_var(node->left, diff_var_idx) || subtree_contains_var(node->right, diff_var_idx);
}

function NODE_T *differentiate_node(const NODE_T *node, size_t diff_var_idx) {
    if (!node) return nullptr;
    switch (node->type) {
        case NUM_T:
            return ZERO();
        case VAR_T:
            return (diff_var_idx != varlist::NPOS && node->value.var == diff_var_idx) ? ONE() : ZERO();
        case OP_T:
            switch (node->value.opr) {
                case ADD: return ADD(dl, dr);
                case SUB: return SUB(dl, dr);
                case MUL: return ADD(MUL(dl, cr), MUL(cl, dr));
                case DIV: return DIV(SUB(MUL(dl, cr), MUL(cl, dr)), MUL(cr, cr));
                case POW: {
                    bool base_const = !subtree_contains_var(node->left, diff_var_idx);
                    bool exp_const  = !subtree_contains_var(node->right, diff_var_idx);
                    if (base_const && exp_const) return ZERO();
                    if (base_const && !exp_const) return MUL(POW(cl, cr), MUL(LN(cl), dr));
                    if (!base_const && exp_const && node->right && node->right->type == NUM_T) {
                        double n = node->right->value.num;
                        return MUL(NUM(n), MUL(POW(cl, NUM(n - 1.0)), dl));
                    }
                    return MUL(POW(cl, cr), ADD(MUL(dr, LN(cl)), DIV(MUL(cr, dl), cl)));
                }
                case LOG: {
                    bool arg_const  = !subtree_contains_var(node->left, diff_var_idx);
                    bool base_const = !subtree_contains_var(node->right, diff_var_idx);
                    if (arg_const && base_const) return ZERO();
                    if (base_const && !arg_const) return DIV(dl, MUL(LN(cr), cl));
                    if (arg_const && !base_const) return MUL(NUM(-1.0), DIV(MUL(LN(cl), dr), MUL(cr, MUL(LN(cr), LN(cr)))));
                    return DIV(SUB(MUL(DIV(dl, cl), LN(cr)), MUL(LN(cl), DIV(dr, cr))), MUL(LN(cr), LN(cr)));
                }
                case LN:   return DIV(dl, cl);
                case SIN:  return MUL(COS(cl), dl);
                case COS:  return MUL(NUM(-1.0), MUL(SIN(cl), dl));
                case TAN:  return DIV(dl, MUL(COS(cl), COS(cl)));
                case CTG:  return MUL(NUM(-1.0), DIV(dl, MUL(SIN(cl), SIN(cl))));
                case ASIN: return DIV(dl, SQRT(SUB(ONE(), MUL(cl, cl))));
                case ACOS: return MUL(NUM(-1.0), DIV(dl, SQRT(SUB(ONE(), MUL(cl, cl)))));
                case ATAN: return DIV(dl, ADD(ONE(), MUL(cl, cl)));
                case ACTG: return MUL(NUM(-1.0), DIV(dl, ADD(ONE(), MUL(cl, cl))));
                case SQRT: return DIV(dl, MUL(NUM(2.0), SQRT(cl)));
                case SINH: return MUL(COSH(cl), dl);
                case COSH: return MUL(SINH(cl), dl);
                case TANH: return DIV(dl, MUL(COSH(cl), COSH(cl)));
                case CTH:  return MUL(NUM(-1.0), DIV(dl, MUL(SINH(cl), SINH(cl))));
                default:  return nullptr;
            }
        default:
            return nullptr;
    }
}

EQ_TREE_T *differentiate(const EQ_TREE_T *src, size_t diff_var_idx) {
    if (!src || !src->root) return nullptr;
    NODE_T *root = differentiate_node(src->root, diff_var_idx);
    if (!root) return nullptr;
    root->parent = nullptr;
    varlist::VarList *vars_copy = src->vars ? varlist::clone(src->vars) : nullptr;
    if (src->vars && !vars_copy) {
        destruct(root);
        return nullptr;
    }
    EQ_TREE_T *res = TYPED_CALLOC(1, EQ_TREE_T);
    if (!res) {
        destruct(root);
        if (vars_copy) {
            varlist::destruct(vars_copy);
            FREE(vars_copy);
        }
        return nullptr;
    }
    const char *original_name = (src->name && *src->name) ? src->name : "equation";
    const mystr::mystr_t *var_entry = (src->vars && diff_var_idx < varlist::size(src->vars))
                                      ? varlist::get(src->vars, diff_var_idx)
                                      : nullptr;
    const char *var_name = (var_entry && var_entry->str && *var_entry->str)
                           ? var_entry->str
                           : "unknown variable";
    const char *prefix = "derivative of ";
    const char *infix = " with respect to ";
    size_t label_len = strlen(prefix) + strlen(original_name) + strlen(infix) + strlen(var_name);
    char *label = TYPED_CALLOC(label_len + 1, char);
    if (!label) {
        destruct(root);
        if (vars_copy) {
            varlist::destruct(vars_copy);
            FREE(vars_copy);
        }
        FREE(res);
        return nullptr;
    }
    snprintf(label, label_len + 1, "%s%s%s%s", prefix, original_name, infix, var_name);
    res->name = label;
    res->root = root;
    res->vars = vars_copy;
    res->owns_vars = vars_copy != nullptr;
    res->owns_name = true;
    return res;
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

