#include "page_rank.h"

#include <cmath>
#include <omp.h>
#include <stdlib.h>
#include <utility>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

// pageRank --
//
// g:           graph to process (see common/graph.h)
// solution:    array of per-vertex vertex scores (length of array is
// num_nodes(g)) damping:     page-rank algorithm's damping parameter
// convergence: page-rank algorithm's convergence threshold
//
void pageRank(Graph g, double *solution, double damping, double convergence) {

  // initialize vertex weights to uniform probability. Double
  // precision scores are used to avoid underflow for large graphs

  int numNodes = num_nodes(g);
  double equal_prob = 1.0 / numNodes;

  bool *no_out_nodes = new bool[numNodes];

#pragma omp parallel for schedule(dynamic, 200)
  for (Vertex i = 0; i < numNodes; ++i) {
    solution[i] = equal_prob;
    no_out_nodes[i] = (outgoing_size(g, i) == 0 ? true : false);
  }

  /*
     CS149 students: Implement the page rank algorithm here.  You
     are expected to parallelize the algorithm using openMP.  Your
     solution may need to allocate (and free) temporary arrays.

     Basic page rank pseudocode is provided below to get you started:
  */
  const double damp_fac = (1.0 - damping) / numNodes;
  bool converged = false;

  double *score_new = new double[numNodes];

  while (!converged) {

    double no_out_sum = damp_fac;
#pragma omp parallel for reduction(+ : no_out_sum) schedule(dynamic, 200)
    for (Vertex i = 0; i < numNodes; ++i) {
      if (no_out_nodes[i]) {
        no_out_sum += damping * solution[i] / numNodes;
      }
    }

// compute score_new[i] for all nodes i:
#pragma omp parallel for schedule(dynamic, 200)
    for (Vertex i = 0; i < numNodes; ++i) {
      score_new[i] = 0;
      const Vertex *pstart = incoming_begin(g, i);
      const Vertex *pend = incoming_end(g, i);
      for (const Vertex *pj = pstart; pj != pend; ++pj) {
        Vertex j = *pj;
        score_new[i] += solution[j] / outgoing_size(g, j);
      }
      score_new[i] = (damping * score_new[i]) + no_out_sum;
    }

    // compute how much per-node scores have changed
    // quit once algorithm has converged

    double global_diff = 0.0;
#pragma omp parallel for reduction(+ : global_diff) schedule(dynamic, 200)
    for (Vertex i = 0; i < numNodes; ++i) {
      global_diff += std::abs(score_new[i] - solution[i]);
      solution[i] = score_new[i];
    }

    converged = global_diff < convergence;
  }

  delete[] score_new;
  delete[] no_out_nodes;
}
