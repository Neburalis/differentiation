#ifndef GRAPH_H
#define GRAPH_H

#include "differentiator.h"

void render_graphs(const FRONT_COMPIL_T *original,
                   const FRONT_COMPIL_T *derivative,
                   const FRONT_COMPIL_T *taylor,
                   double center,
                   size_t var_idx,
                   graph_range_t range
                );

#endif // GRAPH_H