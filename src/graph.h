#ifndef GRAPH_H
#define GRAPH_H

#include "differentiator.h"

void render_graphs(const EQ_TREE_T *original,
                   const EQ_TREE_T *derivative,
                   const EQ_TREE_T *taylor,
                   double center,
                   size_t var_idx,
                   double x_min,
                   double x_max,
                   double y_min,
                   double y_max
                );

#endif // GRAPH_H