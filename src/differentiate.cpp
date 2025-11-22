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
    return copy;
}

function NODE_T *make_number(double value) {
    NODE_VALUE_T val = {.num = value};
    return new_node(NUM_T, val, nullptr, nullptr);
}

function NODE_T *make_binary(OPERATOR op, NODE_T *left, NODE_T *right) {
    NODE_VALUE_T val = {.opr = op};
    NODE_T *node = new_node(OP_T, val, left, right);
    if (!node) {
        destruct(left);
        destruct(right);
    }
    return node;
}

#define ADD(L, R) make_binary(ADD, (L), (R))
#define SUB(L, R) make_binary(SUB, (L), (R))
#define MUL(L, R) make_binary(MUL, (L), (R))
#define DIV(L, R) make_binary(DIV, (L), (R))

#define ZERO() make_number(0.0)
#define ONE() make_number(1.0)

function NODE_T *differentiate_node(const NODE_T *node, size_t diff_var_idx) {
    if (!node) return nullptr;
    switch (node->type) {
        case NUM_T:
            return ZERO();
        case VAR_T:
            return (diff_var_idx != varlist::NPOS && node->value.var == diff_var_idx) ? ONE() : ZERO();
        case OP_T:
            switch (node->value.opr) {
                case ADD: {
                    NODE_T *dl = differentiate_node(node->left, diff_var_idx);
                    NODE_T *dr = differentiate_node(node->right, diff_var_idx);
                    if (!dl || !dr) {
                        destruct(dl);
                        destruct(dr);
                        return nullptr;
                    }
                    return ADD(dl, dr);
                }
                case SUB: {
                    NODE_T *dl = differentiate_node(node->left, diff_var_idx);
                    NODE_T *dr = differentiate_node(node->right, diff_var_idx);
                    if (!dl || !dr) {
                        destruct(dl);
                        destruct(dr);
                        return nullptr;
                    }
                    return SUB(dl, dr);
                }
                case MUL: {
                    NODE_T *dl = differentiate_node(node->left, diff_var_idx);
                    NODE_T *dr = differentiate_node(node->right, diff_var_idx);
                    NODE_T *cl = copy_subtree(node->left);
                    NODE_T *cr = copy_subtree(node->right);
                    if (!dl || !dr || !cl || !cr) {
                        destruct(dl);
                        destruct(dr);
                        destruct(cl);
                        destruct(cr);
                        return nullptr;
                    }
                    return ADD(MUL(dl, cr), MUL(cl, dr));
                }
                case DIV: {
                    NODE_T *dl = differentiate_node(node->left, diff_var_idx);
                    NODE_T *dr = differentiate_node(node->right, diff_var_idx);
                    NODE_T *cl = copy_subtree(node->left);
                    NODE_T *cr = copy_subtree(node->right);
                    NODE_T *cr_a = copy_subtree(node->right);
                    NODE_T *cr_b = copy_subtree(node->right);
                    if (!dl || !dr || !cl || !cr || !cr_a || !cr_b) {
                        destruct(dl);
                        destruct(dr);
                        destruct(cl);
                        destruct(cr);
                        destruct(cr_a);
                        destruct(cr_b);
                        return nullptr;
                    }
                    NODE_T *num = SUB(MUL(dl, cr), MUL(cl, dr));
                    if (!num) {
                        destruct(cr_a);
                        destruct(cr_b);
                        return nullptr;
                    }
                    NODE_T *den = MUL(cr_a, cr_b);
                    if (!den) {
                        destruct(num);
                        return nullptr;
                    }
                    return DIV(num, den);
                }
                default:
                    return nullptr;
            }
        default:
            return nullptr;
    }
}

EQ_TREE_T *differentiate(const EQ_TREE_T *src, size_t diff_var_idx) {
    if (!src || !src->root) return nullptr;
    NODE_T *root = differentiate_node(src->root, diff_var_idx);
    if (!root) return nullptr;
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
    res->name = "d/dx";
    res->root = root;
    res->vars = vars_copy;
    res->owns_vars = vars_copy != nullptr;
    return res;
}

#undef ADD
#undef SUB
#undef MUL
#undef DIV
#undef ZERO
#undef ONE

