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

typedef struct {
    const char         *buf;
    size_t              len;
    size_t              pos;
    bool                error;
    varlist::VarList   *vars;
} parser_t;

// Dumps parser state for verbose debugging output.
function void debug_parse_print(const parser_t *p, const char *reason);

// Extracts the tree name from the parser buffer.
function bool extract_tree_name(parser_t *p, const char **eq_tree_name, char **owned_name);

// Parses a single s-expression node and its children.
function NODE_T *parse_s_expr(parser_t *p);

// Skips whitespace characters in the parser buffer.
function void skip_spaces(parser_t *p);

// Consumes a trailing `nil` child marker when present.
function bool consume_nil(parser_t *p);

// Checks that the next character matches expectation.
function bool expect_char(parser_t *p, char ch, const char *err_fmt);

// Builds a tree node from the current token stream position.
function NODE_T *create_node(parser_t *p);

// Retrieves the next token or reports a parser error.
function char *require_token(parser_t *p);

// Extracts the next standalone token substring.
function char *next_token(parser_t *p);

// Fills node payload depending on deduced node type.
function bool init_node_payload(parser_t *p, NODE_T *node, NODE_TYPE type, char *token);

// Converts numeric literal token into node payload.
function bool store_number(NODE_T *node, const char *token);

// Registers variable name and stores its index.
function bool store_variable(parser_t *p, NODE_T *node, char *token);

// Reads expression file into an equation tree structure.
EQ_TREE_T *load_tree_from_file(const char *filename, const char *eq_tree_name, varlist::VarList *vars) {
    VERIFY(filename != nullptr, ERROR_MSG("filename is nullptr"); return nullptr;);
    VERIFY(vars != nullptr, ERROR_MSG("vars list is nullptr"); return nullptr;);
    char *buffer = read_file_to_buf(filename, nullptr);
    if (!buffer) {
        ERROR_MSG("Can't read expression file '%s'\n", filename);
        return nullptr;
    }
    char *owned_name = nullptr;
    size_t buffer_len = strlen(buffer);
    parser_t parser = {buffer, buffer_len, 0, false, nullptr};
    if (!extract_tree_name(&parser, &eq_tree_name, &owned_name)) {
        FREE(owned_name);
        FREE(buffer);
        return nullptr;
    }
    parser.vars = vars;
    NODE_T *root = parse_s_expr(&parser);
    skip_spaces(&parser);
    while (parser.pos < parser.len) {
        if (!isspace((unsigned char)parser.buf[parser.pos])) {
            parser.error = true;
            ERROR_MSG("Unexpected data after tree at offset %zu\n", parser.pos);
            break;
        }
        ++parser.pos;
    }
    if (parser.error) {
        destruct(root);
        FREE(owned_name);
        FREE(buffer);
        return nullptr;
    }
    FREE(buffer);

    EQ_TREE_T *new_eq_tree = TYPED_CALLOC(1, EQ_TREE_T);

    new_eq_tree->name = eq_tree_name;
    new_eq_tree->root = root;
    new_eq_tree->vars = vars;
    new_eq_tree->owns_name = owned_name != nullptr;
    owned_name = nullptr;

    return new_eq_tree;
}

// Extracts the tree name from the parser buffer.
function bool extract_tree_name(parser_t *p, const char **eq_tree_name, char **owned_name) {
    skip_spaces(p);
    if (p->pos >= p->len || p->buf[p->pos] == '(')
        return true;
    const char *line = p->buf + p->pos;
    const char *line_end = strchr(line, '\n');
    if (!line_end) {
        ERROR_MSG("Name must be on own line\n");
        return false;
    }
    size_t name_len = (size_t)(line_end - line);
    if (name_len > 0) {
        *owned_name = TYPED_CALLOC(name_len + 1, char);
        if (*owned_name) {
            memcpy(*owned_name, line, name_len);
            *eq_tree_name = *owned_name;
        }
    }
    p->pos = (size_t)(line_end - p->buf) + 1;
    skip_spaces(p);
    return true;
}

// Loads a tree with a default name wrapper.
EQ_TREE_T *load_tree_from_file(const char *filename, varlist::VarList *vars) {
    return load_tree_from_file(filename, "New equation tree", vars);
}

#define PARSE_CHILD(p, node, field)                                   \
    do {                                                              \
        skip_spaces(p);                                               \
        (node)->field = parse_s_expr(p);                                \
        if ((p)->error) {                                             \
            destruct(node);                                           \
            return nullptr;                                           \
        }                                                             \
        if ((node)->field) {                                          \
            (node)->field->parent = (node);                           \
            (node)->elements += (node)->field->elements + 1;          \
        }                                                             \
    } while (0)

// Parses a single s-expression node and its children.
function NODE_T *parse_s_expr(parser_t *p) {
    debug_parse_print(p, "parse_s_expr");
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

#undef PARSE_CHILD

// Builds a tree node from the current token stream position.
function NODE_T *create_node(parser_t *p) {
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

// Fills node payload depending on deduced node type.
function bool init_node_payload(parser_t *p, NODE_T *node, NODE_TYPE type, char *token) {
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

// Retrieves the next token or reports a parser error.
function char *require_token(parser_t *p) {
    debug_parse_print(p, "require_token");
    char *token = next_token(p);
    if (token)
        return token;
    PARSE_FAIL(p, "Missing token at position %zu\n", p->pos);
    return nullptr;
}

// Checks that the next character matches expectation.
function bool expect_char(parser_t *p, char ch, const char *err_fmt) {
    debug_parse_print(p, "expect_char");
    if (p->pos >= p->len || p->buf[p->pos] != ch) {
        PARSE_FAIL(p, err_fmt, ch, p->buf[p->pos], p->pos);
        return false;
    }
    ++p->pos;
    return true;
}

// Skips whitespace characters in the parser buffer.
function void skip_spaces(parser_t *p) {
    debug_parse_print(p, "skip_spaces");
    while (p->pos < p->len && isspace((unsigned char)p->buf[p->pos]))
        ++p->pos;
}

// Consumes a trailing `nil` child marker when present.
function bool consume_nil(parser_t *p) {
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

// Extracts the next standalone token substring.
function char *next_token(parser_t *p) {
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

// Converts numeric literal token into node payload.
function bool store_number(NODE_T *node, const char *token) {
    char *end = nullptr;
    double val = strtod(token, &end);
    if (!end || end == token || *end != '\0')
        return false;
    node->value.num = val;
    return true;
}

// Registers variable name and stores its index.
function bool store_variable(parser_t *p, NODE_T *node, char *token) {
    debug_parse_print(p, "store_variable");
    if (!p->vars)
        return false;
    mystr::mystr_t name = mystr::construct(token);
    size_t idx = varlist::add(p->vars, &name);
    return idx != varlist::NPOS ? (node->value.var = idx, true) : false;
}

// Dumps parser state for verbose debugging output.
function void debug_parse_print(const parser_t *p, const char *reason) {
    size_t pos = p->pos < p->len ? p->pos : p->len;
    printf("file loading dump %s\n", reason);
    printf(BRIGHT_BLACK("%.*s"), (int)pos, p->buf);
    if (pos < p->len) {
        printf(GREEN("%c"), p->buf[pos]);
        if (pos + 1 < p->len) printf("%s", p->buf + pos + 1);
    }
    putchar('\n');
}