#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "io_utils.h"
#include "differentiator.h"
#include "base.h"
#include "logger.h"

const char *node_type_name(const NODE_T *node) {
    if (!node) return "UNKNOWN";
    switch (node->type) {
        case NUM_T: return "NUMBER";
        case OP_T:  return "OPERATOR";
        case VAR_T: return "VARIABLE";
        default:    return "UNKNOWN";
    }
}

function const char *node_type_color(const NODE_T *node) {
    if (!node) return "#f2f2f2";
    switch (node->type) {
        case NUM_T: return "#fff2cc";
        case OP_T:  return "#cfe2ff";
        case VAR_T: return "#d4f8d4";
        default:    return "#f2f2f2";
    }
}

function const char *node_type_shape(const NODE_T *node) {
    if (!node) return "hexagon";
    switch (node->type) {
        case NUM_T: return "box";
        case OP_T:  return "ellipse";
        case VAR_T: return "diamond";
        default:    return "hexagon";
    }
}

function const char *node_type_font_size(const NODE_T *node) {
    if (!node) return "10pt";
    switch (node->type) {
        case NUM_T: return "10pt";
        case OP_T:  return "20pt";
        case VAR_T: return "10pt";
        default:    return "10pt";
    }
}

const char *operator_symbol(OPERATOR op) {
    switch (op) {
        case ADD:   return "+";
        case SUB:   return "-";
        case MUL:   return "*";
        case DIV:   return "/";
        case POW:   return "^";
        case LOG:   return "log";
        case LN:    return "ln";
        case SIN:   return "sin";
        case COS:   return "cos";
        case TAN:   return "tan";
        case CTG:   return "ctg";
        case ASIN:  return "arcsin";
        case ACOS:  return "arccos";
        case ATAN:  return "arctan";
        case ACTG:  return "arccot";
        case SQRT:  return "sqrt";
        case SINH:  return "sinh";
        case COSH:  return "cosh";
        case TANH:  return "tanh";
        case CTH:   return "coth";
        default:    return "?";
    }
}

void format_node_value(const EQ_TREE_T *eqtree, const NODE_T *node, char *buf, size_t size) {
    if (!node || !buf || !size) return;
    switch (node->type) {
        case NUM_T:
            snprintf(buf, size, "%.6g", node->value.num);
            break;
        case OP_T:
            snprintf(buf, size, "%s", operator_symbol(node->value.opr));
            break;
        case VAR_T:
            snprintf(buf, size, "%s at [%zu]", eqtree->vars->data[node->value.var].str, node->value.var);
            break;
        default:
            snprintf(buf, size, "-");
            break;
    }
}

function int write_node_full(EQ_TREE_T *eqtree, NODE_T *subtree, FILE *fp, int *id_counter) {
    if (!subtree || !fp || !id_counter) return -1;
    int my_id = (*id_counter)++;

    char value_buf[64] = "";
    format_node_value(eqtree, subtree, value_buf, sizeof(value_buf));

    // const char *color = (subtree == highlight) ? "#ceffb7" : node_type_color(subtree);
    const char *color = node_type_color(subtree);

    if (subtree->elements != 0)
        fprintf(fp,
                "\tnode%d [label=\"{ ptr=%p | type=%s | value=%s | { left=%p | right=%p } | parent=%p | size=%zu }\", shape=record, style=filled, fillcolor=\"%s\"];\n",
                my_id,
                (void *)subtree,
                node_type_name(subtree),
                value_buf,
                (void *)subtree->left,
                (void *)subtree->right,
                (void *)subtree->parent,
                subtree->elements,
                color);
    else
        fprintf(fp,
                "\tnode%d [label=\"{ ptr=%p | type=%s | value=%s | { left=%p | right=%p } | parent=%p }\", shape=record, style=filled, fillcolor=\"%s\"];\n",
                my_id,
                (void *)subtree,
                node_type_name(subtree),
                value_buf,
                (void *)subtree->left,
                (void *)subtree->right,
                (void *)subtree->parent,
                color);

    if (subtree->left) {
        int left_id = write_node_full(eqtree, subtree->left, fp, id_counter);
        if (left_id >= 0)
            fprintf(fp, "\tnode%d -> node%d [color=\"#0c0ccc\", label=\"L\", constraint=true];\n", my_id, left_id);
    }

    if (subtree->right) {
        int right_id = write_node_full(eqtree, subtree->right, fp, id_counter);
        if (right_id >= 0)
            fprintf(fp, "\tnode%d -> node%d [color=\"#3dad3d\", label=\"R\", constraint=true];\n", my_id, right_id);
    }

    return my_id;
}

function int write_node_simple(EQ_TREE_T *eqtree, NODE_T *subtree, FILE *fp, int *id_counter) {
    if (!subtree || !fp || !id_counter) return -1;
    int my_id = (*id_counter)++;

    char value_buf[64] = "";
    format_node_value(eqtree, subtree, value_buf, sizeof(value_buf));

    const char *shape = node_type_shape(subtree);
    const char *color = node_type_color(subtree);
    const char *fontsize = node_type_font_size(subtree);

    fprintf(fp,
            "\tnode%d [label=\"%s\", shape=%s, style=filled, fillcolor=\"%s\", fontsize=\"%s\"];\n",
            my_id,
            value_buf,
            shape,
            color,
            fontsize
        );

    if (subtree->left) {
        int left_id = write_node_simple(eqtree, subtree->left, fp, id_counter);
        if (left_id >= 0)
            fprintf(fp, "\tnode%d -> node%d [color=\"#0c0ccc\", label=\"L\", constraint=true];\n", my_id, left_id);
    }

    if (subtree->right) {
        int right_id = write_node_simple(eqtree, subtree->right, fp, id_counter);
        if (right_id >= 0)
            fprintf(fp, "\tnode%d -> node%d [color=\"#3dad3d\", label=\"R\", constraint=true];\n", my_id, right_id);
    }

    return my_id;
}

function void generate_dot_dump(EQ_TREE_T *eqtree, bool is_simple, FILE *fp) {
    if (!fp) return;
    fprintf(fp,
            "digraph EquationTree {\n"
            "\trankdir=TB;\n"
            "\tnode [fontname=\"Helvetica\", fontsize=10];\n"
            "\tedge [arrowsize=0.8];\n"
            "\tgraph [splines=true, concentrate=false];\n\n");

    if (!eqtree) {
        fprintf(fp, "\t// empty tree\n\n\tlabel = \"empty tree\";\n");
        fprintf(fp, "}\n");
        return;
    }

    int id_counter = 0;
    if (is_simple)
        write_node_simple(eqtree, eqtree->root, fp, &id_counter);
    else
        write_node_full(eqtree, eqtree->root, fp, &id_counter);
    fprintf(fp, "}\n");
}

function int generate_files(EQ_TREE_T *eqtree, bool is_simple, const char *dir, char *out_basename, size_t out_size) {
    local size_t dump_counter = 0;
    const char *outdir = (dir && dir[0] != '\0') ? dir : ".";
    size_t outdir_len = strlen(outdir);

    char dot_path[512] = "";
    if (outdir_len && outdir[outdir_len - 1] == '/')
        snprintf(dot_path, sizeof(dot_path), "%stree_dump_%zu.dot.tmp", outdir, dump_counter);
    else
        snprintf(dot_path, sizeof(dot_path), "%s/tree_dump_%zu.dot.tmp", outdir, dump_counter);

    FILE *fp = fopen(dot_path, "w");
    if (!fp) return -1;

    generate_dot_dump(eqtree, is_simple, fp);
    fclose(fp);

    char image_basename[256] = "";
    snprintf(image_basename, sizeof(image_basename), "tree_dump_%zu.svg", dump_counter);

    char svg_path[512] = "";
    if (outdir_len && outdir[outdir_len - 1] == '/')
        snprintf(svg_path, sizeof(svg_path), "%s%s", outdir, image_basename);
    else
        snprintf(svg_path, sizeof(svg_path), "%s/%s", outdir, image_basename);

    char command[1024] = "";
    snprintf(command, sizeof(command), "dot -Tsvg %s -o %s", dot_path, svg_path);
    (void) system(command);

    if (out_basename && out_size > 0) {
        strncpy(out_basename, image_basename, out_size);
        out_basename[out_size - 1] = '\0';
    }

    ++dump_counter;
    return 0;
}

function void dump_internal(EQ_TREE_T *eqtree, bool is_simple, const char *fmt, va_list ap) {
    const char *dir = logger_get_active_dir();
    char basename[256] = "";
    int rc = generate_files(eqtree, is_simple, dir, basename, sizeof(basename));

    FILE *log_file = logger_get_file();
    if (!log_file)
        return;

    if (fmt != nullptr) {
        fprintf(log_file, "<p>");
        vfprintf(log_file, fmt, ap);
        fprintf(log_file, "</p>\n");
        fflush(log_file);
    }
    else {
        fprintf(log_file, "<p>Dump of %s</p>", eqtree->name);
        fflush(log_file);
    }

    if (rc == 0 && basename[0] != '\0')
        fprintf(log_file, "<img src=\"%s\">\n", basename);
    else
        fprintf(log_file, "<p>SVG not generated</p>\n");

    fflush(log_file);
}

void full_dump(EQ_TREE_T *node) {
    va_list ap = {};
    dump_internal(node, false, nullptr, ap);
}

void full_dump(EQ_TREE_T *node, const char *fmt, ...) {
    va_list ap = {};
    va_start(ap, fmt);
    dump_internal(node, false, fmt, ap);
    va_end(ap);
}

void simple_dump(EQ_TREE_T *node) {
    va_list ap = {};
    dump_internal(node, true, nullptr, ap);
}

void simple_dump(EQ_TREE_T *node, const char *fmt, ...) {
    va_list ap = {};
    va_start(ap, fmt);
    dump_internal(node, true, fmt, ap);
    va_end(ap);
}

function int latex_prec_op(OPERATOR op) {
    switch (op) {
        case ADD:
        case SUB:   return 1;
        case MUL:
        case DIV:   return 2;
        case POW:   return 3;
        default:    return 4;
    }
}

function int latex_prec(const NODE_T *node) {
    if (!node || node->type != OP_T) return 4;
    return latex_prec_op(node->value.opr);
}

function bool need_parens(OPERATOR parent, const NODE_T *child, bool right_side) {
    if (!child || child->type != OP_T) return false;
    if (parent == DIV) return false;
    int parent_prec = latex_prec_op(parent);
    int child_prec = latex_prec(child);
    if (parent == POW) {
        return right_side ? (child_prec <= parent_prec) : (child_prec < parent_prec);
    }
    if (child_prec < parent_prec) return true;
    if (child_prec > parent_prec) return false;
    if (parent == SUB && right_side) return true;
    return false;
}

function const char *latex_func_name(OPERATOR op) {
    switch (op) {
        case LN:   return "\\ln";
        case SIN:  return "\\sin";
        case COS:  return "\\cos";
        case TAN:  return "\\tan";
        case CTG:  return "\\cot";
        case ASIN: return "\\arcsin";
        case ACOS: return "\\arccos";
        case ATAN: return "\\arctan";
        case ACTG: return "\\arccot";
        case SINH: return "\\sinh";
        case COSH: return "\\cosh";
        case TANH: return "\\tanh";
        case CTH:  return "\\coth";
        default:   return nullptr;
    }
}

function size_t latex_size(EQ_TREE_T *tree, NODE_T *node) {
    if (!tree || !node) return 0;
    switch (node->type) {
        case NUM_T: {
            char buf[64] = "";
            int written = snprintf(buf, sizeof(buf), "%.6g", node->value.num);
            return written > 0 ? (size_t) written : 0;
        }
        case VAR_T: {
            const mystr::mystr_t *name = tree->vars ? varlist::get(tree->vars, node->value.var) : nullptr;
            return (name && name->str) ? strlen(name->str) : 0;
        }
        case OP_T: {
            size_t left_len = node->left ? latex_size(tree, node->left) : 0;
            size_t right_len = node->right ? latex_size(tree, node->right) : 0;
            switch (node->value.opr) {
                case DIV:
                    return left_len + right_len + 9;
                case ADD:
                case SUB: {
                    size_t extra = (need_parens(node->value.opr, node->left, false) ? 2 : 0) +
                                   (need_parens(node->value.opr, node->right, true) ? 2 : 0);
                    return left_len + right_len + extra + 3;
                }
                case MUL: {
                    size_t extra = (need_parens(node->value.opr, node->left, false) ? 2 : 0) +
                                   (need_parens(node->value.opr, node->right, true) ? 2 : 0);
                    return left_len + right_len + extra + 7;
                }
                case POW: {
                    size_t extra = (need_parens(node->value.opr, node->left, false) ? 2 : 0);
                    size_t exp_extra = (need_parens(node->value.opr, node->right, true) ? 2 : 0);
                    if (right_len == 0) return left_len + extra;
                    return left_len + right_len + extra + exp_extra + 3;
                }
                case LOG: {
                    size_t base_part = 0;
                    if (node->right && right_len) base_part = 3 + right_len; // _{ base }
                    return 4 + base_part + 2 + left_len;
                }
                case LN:
                case SIN:
                case COS:
                case TAN:
                case CTG:
                case ASIN:
                case ACOS:
                case ATAN:
                case ACTG:
                case SINH:
                case COSH:
                case TANH:
                case CTH: {
                    const char *name = latex_func_name(node->value.opr);
                    size_t name_len = name ? strlen(name) : 0;
                    return name_len + 2 + left_len;
                }
                case SQRT:
                    return 6 + left_len + 1;
                default:
                    return left_len + right_len;
            }
        }
        default:
            return 0;
    }
}

function void append_char(char **dst, char ch) {
    if (!dst || !*dst) return;
    **dst = ch;
    ++(*dst);
}

function void append_str(char **dst, const char *src) {
    if (!dst || !*dst || !src) return;
    size_t len = strlen(src);
    memcpy(*dst, src, len);
    *dst += len;
}

function void latex_emit(EQ_TREE_T *tree, NODE_T *node, char **out) {
    if (!tree || !node || !out || !*out) return;
    switch (node->type) {
        case NUM_T: {
            char buf[64] = "";
            int written = snprintf(buf, sizeof(buf), "%.6g", node->value.num);
            if (written > 0) append_str(out, buf);
            break;
        }
        case VAR_T: {
            const mystr::mystr_t *name = tree->vars ? varlist::get(tree->vars, node->value.var) : nullptr;
            if (name && name->str) append_str(out, name->str);
            break;
        }
        case OP_T: {
            switch (node->value.opr) {
                case DIV:
                    append_str(out, "\\frac{");
                    latex_emit(tree, node->left, out);
                    append_str(out, "}{");
                    latex_emit(tree, node->right, out);
                    append_char(out, '}');
                    return;
                case POW: {
                    bool wrap_left = need_parens(node->value.opr, node->left, false);
                    bool wrap_right = need_parens(node->value.opr, node->right, true);
                    if (wrap_left) append_char(out, '(');
                    latex_emit(tree, node->left, out);
                    if (wrap_left) append_char(out, ')');
                    if (node->right) {
                        append_str(out, "^{");
                        if (wrap_right) append_char(out, '(');
                        latex_emit(tree, node->right, out);
                        if (wrap_right) append_char(out, ')');
                        append_char(out, '}');
                    }
                    return;
                }
                case LOG:
                    append_str(out, "\\log");
                    if (node->right) {
                        append_str(out, "_{");
                        latex_emit(tree, node->right, out);
                        append_char(out, '}');
                    }
                    append_char(out, '{');
                    latex_emit(tree, node->left, out);
                    append_char(out, '}');
                    return;
                case SQRT:
                    append_str(out, "\\sqrt{");
                    latex_emit(tree, node->left, out);
                    append_char(out, '}');
                    return;
                default: {
                    const char *func = latex_func_name(node->value.opr);
                    if (func) {
                        append_str(out, func);
                        append_char(out, '{');
                        latex_emit(tree, node->left, out);
                        append_char(out, '}');
                        return;
                    }
                }
            }
            bool wrap_left = need_parens(node->value.opr, node->left, false);
            bool wrap_right = need_parens(node->value.opr, node->right, true);
            if (wrap_left) append_char(out, '(');
            latex_emit(tree, node->left, out);
            if (wrap_left) append_char(out, ')');
            switch (node->value.opr) {
                case ADD: append_str(out, " + "); break;
                case SUB: append_str(out, " - "); break;
                case MUL: append_str(out, " \\cdot "); break;
                default: break;
            }
            if (wrap_right) append_char(out, '(');
            latex_emit(tree, node->right, out);
            if (wrap_right) append_char(out, ')');
            break;
        }
        default:
            break;
    }
}

// U need to free the returned string
char *latex_dump(EQ_TREE_T *node) {
    if (!node || !node->root) {
        ERROR_MSG("latex_dump: node or node->root is nullptr");
        return nullptr;
    }
    size_t len = latex_size(node, node->root);
    char *buf = TYPED_CALLOC(len + 1, char);
    if (!buf) return nullptr;
    char *cursor = buf;
    latex_emit(node, node->root, &cursor);
    if (cursor) *cursor = '\0';
    return buf;
}