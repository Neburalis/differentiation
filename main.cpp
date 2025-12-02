#include <stdio.h>
#include <time.h>

#include "differentiator.h"
#include "logger.h"
#include "io_utils.h"
#include "base.h"
#include "const_strings.h"

const char * HABR_ARTICLE_FILENAME = "logs/habr.html";

int main(int argc, char *argv[]) {
    srand(time(nullptr));

    create_folder_if_not_exists("logs/");
    init_logger(
        "logs/",
        "\n<script>window.MathJax={tex:{inlineMath:[['$','$'],['\\\\(','\\\\)']],displayMath:[['$$','$$'],['\\\\[','\\\\]']]},"
        "options:{skipHtmlTags:['script','noscript','style','textarea','annotation','annotation-xml']}};</script>"
        "\n<script src=\"https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js\" async></script>"
        "\n<style>pre{white-space:pre-wrap;font-family:monospace}</style>");

    fprintf(logger_get_file(), "<H2>MEOW!</H2>");

    if (argc <= 1) {
        ERROR_MSG(RED("U must provide path to source equation text\n")"eq_calc path_to_equation\n");
        return 1;
    }
    else if (argc > 2) {
        ERROR_MSG(RED("U must provide only one path to source equation text\n")"eq_calc path_to_equation\n");
        return 1;
    }

    char *equation_filename = argv[1];

    TERMINAL_ENTER_ALT_SCREEN();

    varlist::VarList var_list = {};
    EQ_TREE_T *tree = load_tree_from_file(equation_filename, &var_list);
    if (!tree) {
        printf("Failed to load tree from file\n");
        return 1;
    }

    // printf("name is [%s]\n", tree->name);
    // getchar();
    TERMINAL_EXIT_ALT_SCREEN();

    FILE *habr_article = fopen(HABR_ARTICLE_FILENAME, "w");
    system("cat habr_page/habr_start.html >> logs/habr.html");
    fseek(habr_article, 0, SEEK_END);
    fprintf(habr_article, "<p>%s</p>\n<br>", INTRO_STR);
    differentiate_set_article_file(habr_article);

    full_dump(tree, "Dump from line %d", 18);
    simple_dump(tree, "Simple dump from line 20");

    // EQ_POINT_T point = read_point_data(tree);
    // calc_in_point(&point);
    // printf("Result: %.10g\n", point.result);

    mystr::mystr_t x_str = mystr::construct("x");
    EQ_TREE_T *first_derivative = differentiate(tree, varlist::find_index(tree->vars, &x_str));
    full_dump(first_derivative, "First derivative from line %d", __LINE__);
    simple_dump(first_derivative, "Simple first derivative from line %d", __LINE__);

    // point.tree = first_derivative;
    // calc_in_point(&point);
    // printf("first_derivative Result in point: %.10g\n", point.result);

    char *latex_origin = latex_dump(tree);
    char *latex_first  = latex_dump(first_derivative);

    fprintf(differentiate_get_article_stream(),
            "<hr><p>%s</p>\n"
            "$$\\frac{\\mathrm{d}}{\\mathrm{d}x} %s = %s$$",
            RESULT_STR[randint(0, ARRAY_COUNT(RESULT_STR))],
            latex_origin,
            latex_first
        );
    // fprintf(stdout,
    //         "<hr><p>%s</p>\n"
    //         "$$\frac{\mathrm{d}}{\mathrm{d}x} %s = %s$$",
    //         RESULT_STR[randint(0, ARRAY_COUNT(RESULT_STR))],
    //         latex_origin,
    //         latex_first
    //     );

    FREE(latex_origin);
    FREE(latex_first);

    // FREE(point.point);
    destruct(first_derivative);
    destruct(tree);
    varlist::destruct(&var_list);
    destruct_logger();


    fprintf(habr_article, "\n\n<br>\n<p>%s</p>\n", CONCLUSION_STR);
    fflush(habr_article);
    system("cat habr_page/habr_end.html >> logs/habr.html");
    fclose(habr_article);
    char command[256] = {};
    snprintf(command, sizeof(command), "open %s", HABR_ARTICLE_FILENAME);
    system(command);

    // getchar();
    // getchar();
    return 0;
}