#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "differentiator.h"
#include "logger.h"
#include "io_utils.h"
#include "base.h"
#include "const_strings.h"

const char * LATEX_SOURCE_FILENAME = "logs/report.tex";
const char * LATEX_OUTPUT_FILENAME = "logs/report.pdf";
const size_t COUNT_OF_DIFFS = 4;

const double TANGENT_POINT = 0;

const double X_MIN = -5;
const double X_MAX =  5;
const double Y_MIN = -5;
const double Y_MAX =  5;

int main(int argc, char *argv[]) {
    srand(time(nullptr));

    create_folder_if_not_exists("logs/");
    init_logger("logs/");

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
    getchar();
    TERMINAL_EXIT_ALT_SCREEN();

    FILE *latex_article = fopen(LATEX_SOURCE_FILENAME, "w");
    if (!latex_article) {
        ERROR_MSG(RED("Failed to create LaTeX file %s\n"), LATEX_SOURCE_FILENAME);
        destruct(tree);
        varlist::destruct(&var_list);
        destruct_logger();
        return 1;
    }

    fprintf(latex_article, LATEX_BEGIN, INTRO_STR);
    differentiate_set_article_file(latex_article);

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
            "\\bigskip\\hrule\\bigskip\n"
            "%s\n\n"
            "\\begin{dmath*}\n"
            "\\frac{\\mathrm{d}}{\\mathrm{d}x} %s = %s\n"
            "\\end{dmath*}\n",
            RESULT_STR[randint(0, ARRAY_COUNT(RESULT_STR))],
            latex_origin,
            latex_first);
    // fprintf(stdout,
    //         "<hr><p>%s</p>\n"
    //         "$$\frac{\mathrm{d}}{\mathrm{d}x} %s = %s$$",
    //         RESULT_STR[randint(0, ARRAY_COUNT(RESULT_STR))],
    //         latex_origin,
    //         latex_first
    //     );

    // EQ_TREE_T **dif_array = differentiate_to_n(tree, COUNT_OF_DIFFS, varlist::find_index(tree->vars, &x_str));

    // article_log_text("<hr><p>Теперь быстренько пробежимся по остальным производным:</p>");
    // article_log_text("<details>\n<summary>Все производные:</summary>");
    // for (size_t i = 2; i <= COUNT_OF_DIFFS; ++i) {
    //     char *latex = latex_dump(dif_array[i]);
    //     int font_size = 1;
    //     article_log_text("Производная #%zu: \n$$f^{(%zu)} = %s $$", i, i, latex);
    //     FREE(latex);
    // }
    // article_log_text("</details>");

    // destruct(dif_array);

    FREE(latex_origin);
    FREE(latex_first);

    // FREE(point.point);
    destruct(first_derivative);
    destruct(tree);
    varlist::destruct(&var_list);
    destruct_logger();

    fprintf(latex_article, "\n\\bigskip\\hrule\\bigskip\n%s\n\n\\end{document}\n", CONCLUSION_STR);
    fclose(latex_article);
    differentiate_set_article_file(nullptr);

    char compile_cmd[512] = {};
    snprintf(compile_cmd, sizeof(compile_cmd),
             "tectonic --outdir logs --chatter minimal %s > /dev/null",
             LATEX_SOURCE_FILENAME);
    int compile_status = system(compile_cmd);
    if (compile_status == 0) {
        char open_cmd[256] = {};
        snprintf(open_cmd, sizeof(open_cmd), "open %s", LATEX_OUTPUT_FILENAME);
        system(open_cmd);
    } else {
        ERROR_MSG(RED("tectonic failed, see logs for details\n"));
    }

    // getchar();
    // getchar();
    return 0;
}