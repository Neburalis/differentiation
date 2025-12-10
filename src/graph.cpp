#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "base.h"
#include "differentiator.h"
#include "graph.h"
#include "io_utils.h"
#include "var_list.h"

static double eval_tree_value(const EQ_TREE_T *tree, size_t var_idx, double x) {
    if (!tree || !tree->root) return NAN;
    size_t vars_count = tree->vars ? varlist::size(tree->vars) : 0;
    double *values = nullptr;
    if (vars_count > 0) {
        values = (double *)calloc(vars_count, sizeof(double));
        if (!values) return NAN;
        if (var_idx < vars_count) values[var_idx] = x;
    }
    EQ_POINT_T point = {.tree = tree, .point = values, .vars_count = vars_count, .result = 0.0};
    calc_in_point(&point);
    double res = point.result;
    FREE(values);
    return res;
}

void render_graphs(const EQ_TREE_T *original,
                   const EQ_TREE_T *derivative,
                   const EQ_TREE_T *taylor,
                   double center,
                   size_t var_idx,
                   graph_range_t range
                ) {
    double x_min = NAN, x_max = NAN, y_min = NAN, y_max = NAN;
    if (!isfinite(range.x_min) || !isfinite(range.x_max)) {
        x_min = -1.1, x_max = 1.1;
    }
    else {
        x_min = range.x_min, x_max = range.x_max;
    }
    y_min = range.y_min, y_max = range.y_max;

    if (!original || !original->root || x_min >= x_max) return;

    // printf("xrange [%lg:%lg]\n"
    //     "yrange [%lg:%lg]\n",
    //     x_min, x_max,
    //     y_min, y_max
    //  );

    create_folder_if_not_exists("logs/");

    const size_t samples = 400;
    double step = (x_max - x_min) / (samples - 1);
    bool has_derivative = derivative && derivative->root;
    bool has_taylor = taylor && taylor->root;
    double f_center = eval_tree_value(original, var_idx, center);
    double slope = has_derivative ? eval_tree_value(derivative, var_idx, center) : NAN;
    bool has_tangent = has_derivative && isfinite(f_center) && isfinite(slope);

    FILE *data = fopen("logs/graph_data.dat", "w");
    if (!data) {
        ERROR_MSG("failed to open logs/graph_data.dat\n");
        return;
    }
    fprintf(data, "# x\tf(x)\ttangent(x)\ttaylor(x)\n");
    for (size_t i = 0; i < samples; ++i) {
        double x = x_min + step * i;
        double fx = eval_tree_value(original, var_idx, x);
        double tangent = has_tangent ? f_center + slope * (x - center) : NAN;
        double tx = has_taylor ? eval_tree_value(taylor, var_idx, x) : NAN;
        // printf("x = %lg, fx = %lg, tan = %lg, tx = %lg\n", x, fx, tangent, tx);
        fprintf(data, "%.10g %.10g %.10g %.10g\n", x, fx, tangent, tx);
    }
    fclose(data);

    FILE *script = fopen("logs/graph_plot.gnu", "w");
    if (!script) {
        ERROR_MSG("failed to open logs/graph_plot.gnu\n");
        return;
    }
    // if (!isfinite(y_min) || !isfinite(y_max))
    fprintf(script,
            "set terminal pngcairo size 1280,720\n"
            "set output 'logs/graphs.png'\n"
            "set title 'Графики функции и аппроксимаций'\n"
            "set xlabel 'x'\n"
            "set ylabel 'y'\n"
            "set xrange [%g:%g]\n"
            "set yrange [%g:%g]\n"
            "set pointsize 2\n"
            "set grid\n"
            "set key outside top center horizontal\n",
            x_min, x_max, y_min, y_max);
    // else
    // fprintf(script,
    //         "set terminal pngcairo size 1280,720\n"
    //         "set output 'logs/graphs.png'\n"
    //         "set title 'Графики функции и аппроксимаций'\n"
    //         "set xlabel 'x'\n"
    //         "set ylabel 'y'\n"
    //         "set xrange [%g:%g]\n"
    //         "set grid\n"
    //         "set key outside top center horizontal\n",
    //         x_min, x_max);

    fprintf(script,
            "plot \\\n"
            "  'logs/graph_data.dat' using 1:2 with lines lw 2 lc rgb '#1f77b4' title 'f(x)'");
    if (has_tangent) {
        fprintf(script,
                ", \\\n  'logs/graph_data.dat' using 1:3 with lines lw 2 lc rgb '#d62728' "
                "title 'Касательная в x=%.3g', \"<echo '%lg %lg'\" with points",
                center, center, f_center);
    }
    if (has_taylor) {
        fprintf(script,
                ", \\\n  'logs/graph_data.dat' using 1:4 with lines lw 2 lc rgb '#2ca02c' "
                "title 'Полином Тейлора'");
    }
    fprintf(script, "\n");
    fclose(script);

    int status = system("gnuplot logs/graph_plot.gnu > /dev/null 2>&1");
    if (status != 0) {
        ERROR_MSG("gnuplot exited with code %d\n", status);
    }
}

