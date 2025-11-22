#include <stdio.h>

#include "differentiator.h"
#include "logger.h"
#include "io_utils.h"
#include "base.h"

int main() {
    create_folder_if_not_exists("logs/");
    init_logger(
        "logs/",
        "\n<script>window.MathJax={tex:{inlineMath:[['$','$'],['\\\\(','\\\\)']],displayMath:[['$$','$$'],['\\\\[','\\\\]']]},"
        "options:{skipHtmlTags:['script','noscript','style','textarea','annotation','annotation-xml']}};</script>"
        "\n<script src=\"https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js\" async></script>"
        "\n<style>pre{white-space:pre-wrap;font-family:monospace}</style>");

    fprintf(logger_get_file(), "<H2>MEOW!</H2>");

    varlist::VarList var_list = {};
    varlist::init(&var_list);

    EQ_TREE_T *tree = load_tree_from_file("test_expression.txt", &var_list);

    full_dump(tree, "Dump from line %d", 18);

    simple_dump(tree, "Simple dump from line 20");

    char *latex = latex_dump(tree);
    fprintf(logger_get_file(), "\n$$%s$$\n", latex);
    printf("LaTeX: %s\n", latex);
    FREE(latex);

    EQ_POINT_T point = read_point_data(tree);
    calc_in_point(&point);
    printf("Result: %.10g\n", point.result);

    mystr::mystr_t x_str = mystr::construct("x");
    EQ_TREE_T *first_derivative = differentiate(tree, varlist::find_index(tree->vars, &x_str));
    full_dump(first_derivative, "First derivative from line %d", 38);
    simple_dump(first_derivative, "Simple first derivative from line %d", 39);
    latex = latex_dump(first_derivative);
    fprintf(logger_get_file(), "\n$$%s$$\n", latex);
    printf("first_derivative LaTeX: %s\n", latex);
    FREE(latex);

    calc_in_point(&point);
    printf("first_derivative Result in point: %.10g\n", point.result);

    FREE(point.point);
    destruct(first_derivative);
    destruct(tree);
    varlist::destruct(&var_list);
    destruct_logger();

    return 0;
}