#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "differentiator.h"
#include "logger.h"
#include "io_utils.h"
#include "base.h"
#include "const_strings.h"
#include "graph.h"
#include "article.h"

const char * LATEX_SOURCE_FILENAME = "logs/report.tex";
const char * LATEX_OUTPUT_FILENAME = "logs/report.pdf";
const size_t COUNT_OF_DIFFS = 7;

const double TAILOR_POINT = 1;

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
    graph_range_t range = {};
    EQ_TREE_T *tree = load_tree_from_file(equation_filename, &var_list, &range);
    if (!tree) {
        printf("Failed to load tree from file\n");
        return 1;
    }

//     printf("xrange [%lg:%lg]\n"
//            "yrange [%lg:%lg]\n",
//            range.x_min, range.x_max,
//            range.y_min, range.y_max
//         );
//
//     printf("name is [%s]\n", tree->name);
//     getchar();
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
    article_log_text("\\tableofcontents");

    full_dump(tree, "Dump from line %d", 18);
    simple_dump(tree, "Simple dump from line 20");

    // EQ_POINT_T point = read_point_data(tree);
    // calc_in_point(&point);
    // printf("Result: %.10g\n", point.result);

    article_log_text("\\bigskip\\hrule\\bigskip\n\\section{Посчитаем производную}");
    // article_log_text("\\bigskip\\hrule\\bigskip\n\\subsection*{Первая производная}");

    mystr::mystr_t x_str = mystr::construct("x");
    size_t x_var_idx = varlist::find_index(tree->vars, &x_str);
    differentiate_set_article_file(nullptr);
    EQ_TREE_T *first_derivative = differentiate(tree, x_var_idx);
    differentiate_set_article_file(latex_article);
    full_dump(first_derivative, "First derivative from line %d", __LINE__);
    simple_dump(first_derivative, "Simple first derivative from line %d", __LINE__);

    // printf("Put enter ...\n");
    // getchar();

    // point.tree = first_derivative;
    // calc_in_point(&point);
    // printf("first_derivative Result in point: %.10g\n", point.result);

    // article_log_text("%s\nРезультат %s производной:",
    //                  RESULT_STR[randint(0, ARRAY_COUNT(RESULT_STR))],
    //                  first_ord ? first_ord : "первой");
    // article_log_with_latex(first_derivative, nullptr);

    EQ_TREE_T **dif_array = differentiate_to_n(tree, COUNT_OF_DIFFS, x_var_idx);
    EQ_TREE_T *tailor = nullptr;

    // article_log_text("\\bigskip\\hrule\\bigskip\n\\section*{Первые %zu производных}", COUNT_OF_DIFFS);
    // for (size_t i = 2; i <= COUNT_OF_DIFFS; ++i) {
    //     article_log_text("\\bigskip\\hrule\\bigskip\n\\subsection*{%zu производная}", i);
    //     char *latex = latex_dump(dif_array[i]);
    //     if (!latex) continue;
    //     // printf("f^(%zu) = %s\n", i, latex);
    //     article_log_text(
    //         // "Производная №%zu\n"
    //         "\\begin{dmath*}\nf^{(%zu)}(x) = %s\n\\end{dmath*}", i, latex);
    //     FREE(latex);
    // }

    article_log_text("\\newpage");
    article_log_text("\\section{Формула Тейлора}");
    article_log_text("Разложение функции в окрестности x = %lg:", TAILOR_POINT);
    tailor = tailor_formula(dif_array, COUNT_OF_DIFFS, TAILOR_POINT, x_var_idx);
    // article_log_with_latex(tailor, nullptr);

    render_graphs(tree, first_derivative, tailor, TAILOR_POINT, x_var_idx, range);

    article_log_text("\\section{График}");
    article_log_text("\\begin{figure}[h]\\centering\\includegraphics[width=0.9\\textwidth]{graphs.png}\\caption{Графики функции и аппроксимаций}\\end{figure}");

    if (dif_array) destruct(dif_array);
    if (tailor) destruct(tailor);

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