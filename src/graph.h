#ifndef GRAPH_H
#define GRAPH_H

#include "differentiator.h"

void render_graphs(const EQ_TREE_T *original,
                   const EQ_TREE_T *derivative,
                   const EQ_TREE_T *taylor,
                   double center,
                   size_t var_idx,
                   graph_range_t range
                );

#endif // GRAPH_H