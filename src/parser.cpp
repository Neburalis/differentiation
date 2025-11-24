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

#define PARSE_FAIL(p, ...)            \
    do {                              \
        (p)->error = true;            \
        ERROR_MSG(__VA_ARGS__);       \
    } while (0)

#define PARSE_CHILD(p, node, field)                                   \
    do {                                                              \
        skip_spaces(p);                                               \
        (node)->field = parse_node(p);                                \
        if ((p)->error) {                                             \
            destruct(node);                                           \
            return nullptr;                                           \
        }                                                             \
        if ((node)->field) {                                          \
            (node)->field->parent = (node);                           \
            (node)->elements += (node)->field->elements + 1;          \
        }                                                             \
    } while (0)

typedef struct {
    const char         *buf;
    size_t              len;
    size_t              pos;
    bool                error;
    varlist::VarList   *vars;
} Parser;

function void debug_parse_print(const Parser *p, const char *reason);

function void skip_spaces(Parser *p) {
    debug_parse_print(p, "skip_spaces");
    while (p->pos < p->len && isspace((unsigned char)p->buf[p->pos]))
        ++p->pos;
}

function bool consume_nil(Parser *p) {
    debug_parse_print(p, "consume_nil");
    if (p->pos + 3 > p->len)
        return false;
    if (strncmp(p->buf + p->pos, "nil", 3) != 0)
        return false;
    size_t next = p->pos + 3;
    if (next < p->len) {
        char c = p->buf[next];
        if (!isspace((unsigned char)c) && c != ')')
            return false;
    }
    p->pos = next;
    return true;
}

function char *next_token(Parser *p) {
    debug_parse_print(p, "next_token");
    skip_spaces(p);
    size_t start = p->pos;
    while (p->pos < p->len) {
        char c = p->buf[p->pos];
        if (isspace((unsigned char)c) || c == '(' || c == ')')
            break;
        ++p->pos;
    }
    if (start == p->pos)
        return nullptr;
    size_t span = p->pos - start;
    char *token = TYPED_CALLOC(span + 1, char);
    if (!token)
        return nullptr;
    memcpy(token, p->buf + start, span);
    token[span] = '\0';
    return token;
}

function bool store_number(NODE_T *node, const char *token) {
    char *end = nullptr;
    double val = strtod(token, &end);
    if (!end || end == token || *end != '\0')
        return false;
    node->value.num = val;
    return true;
}

function bool store_variable(Parser *p, NODE_T *node, char *token) {
    debug_parse_print(p, "store_variable");
    if (!p->vars)
        return false;
    mystr::mystr_t name = mystr::construct(token);
    size_t idx = varlist::add(p->vars, &name);
    return idx != varlist::NPOS ? (node->value.var = idx, true) : false;
}

function void debug_parse_print(const Parser *p, const char *reason) {
    size_t pos = p->pos < p->len ? p->pos : p->len;
    printf("file loading dump %s\n", reason);
    printf(BRIGHT_BLACK("%.*s"), (int)pos, p->buf);
    if (pos < p->len) {
        printf(GREEN("%c"), p->buf[pos]);
        if (pos + 1 < p->len) printf("%s", p->buf + pos + 1);
    }
    putchar('\n');
}

function bool expect_char(Parser *p, char ch, const char *err_fmt) {
    debug_parse_print(p, "expect_char");
    if (p->pos >= p->len || p->buf[p->pos] != ch) {
        PARSE_FAIL(p, err_fmt, ch, p->buf[p->pos], p->pos);
        return false;
    }
    ++p->pos;
    return true;
}

function char *require_token(Parser *p) {
    debug_parse_print(p, "require_token");
    char *token = next_token(p);
    if (token)
        return token;
    PARSE_FAIL(p, "Missing token at position %zu\n", p->pos);
    return nullptr;
}

function bool init_node_payload(Parser *p, NODE_T *node, NODE_TYPE type, char *token) {
    bool ok = false;
    switch (type) {
        case NUM_T: ok = store_number(node, token); break;
        case OP_T: ok = operator_from_token(token, &node->value.opr); break;
        case VAR_T: ok = store_variable(p, node, token); break;
        default:
            PARSE_FAIL(p, "Unknown node type: %d\n", type);
            return false;
    }
    if (!ok)
        PARSE_FAIL(p, "Failed to interpret node payload\n");
    return ok;
}

function NODE_T *create_node(Parser *p) {
    debug_parse_print(p, "create_node");
    char *token = require_token(p);
    if (!token)
        return nullptr;
    NODE_TYPE type;
    if (!node_type_from_token(token, &type)) {
        PARSE_FAIL(p, "Unknown token '%s'\n", token);
        free(token);
        return nullptr;
    }
    NODE_T *node = alloc_new_node();
    if (!node) {
        PARSE_FAIL(p, "Allocation failed while parsing '%s'\n", token);
        free(token);
        return nullptr;
    }
    node->type = type;
    if (!init_node_payload(p, node, type, token)) {
        free(token);
        free(node);
        return nullptr;
    }
    free(token);
    return node;
}

function NODE_T *parse_node(Parser *p) {
    debug_parse_print(p, "parse_node");
    skip_spaces(p);
    if (p->pos >= p->len) {
        PARSE_FAIL(p, "Unexpected end of input while parsing node\n");
        return nullptr;
    }
    if (consume_nil(p))
        return nullptr;
    if (!expect_char(p, '(', "Expected '%c', actual '%c' at position %zu\n"))
        return nullptr;
    NODE_T *node = create_node(p);
    if (!node)
        return nullptr;
    PARSE_CHILD(p, node, left);
    PARSE_CHILD(p, node, right);
    skip_spaces(p);
    if (!expect_char(p, ')', "Expected '%c', actual '%c' at position %zu\n")) {
        destruct(node);
        return nullptr;
    }
    return node;
}

EQ_TREE_T *load_tree_from_file(const char *filename, const char *eq_tree_name, varlist::VarList *vars) {
    VERIFY(filename != nullptr, ERROR_MSG("filename is nullptr"); return nullptr;);
    VERIFY(vars != nullptr, ERROR_MSG("vars list is nullptr"); return nullptr;);
    char *buffer = read_file_to_buf(filename, nullptr);
    if (!buffer) {
        ERROR_MSG("Can't read expression file '%s'\n", filename);
        return nullptr;
    }
    Parser parser = {buffer, strlen(buffer), 0, false, vars};
    NODE_T *root = parse_node(&parser);
    skip_spaces(&parser);
    if (!parser.error) {
        while (parser.pos < parser.len) {
            if (!isspace((unsigned char)parser.buf[parser.pos])) {
                parser.error = true;
                ERROR_MSG("Unexpected data after tree at offset %zu\n", parser.pos);
                break;
            }
            ++parser.pos;
        }
    }
    if (parser.error) {
        destruct(root);
        free(buffer);
        return nullptr;
    }
    free(buffer);

    EQ_TREE_T *new_eq_tree = TYPED_CALLOC(1, EQ_TREE_T);

    new_eq_tree->name = eq_tree_name;
    new_eq_tree->root = root;
    new_eq_tree->vars = vars;

    return new_eq_tree;
}

EQ_TREE_T *load_tree_from_file(const char *filename, varlist::VarList *vars) {
    return load_tree_from_file(filename, "New equation tree", vars);
}