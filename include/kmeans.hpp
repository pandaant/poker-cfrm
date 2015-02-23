#ifndef KMEANS_HPP
#define KMEANS_HPP

#include <vector>
#include <cstdlib>
#include <math.h>
#include <algorithm>
#include <boost/static_assert.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>

#define NULL 0
#include "emd_hat.hpp"
#include "definitions.hpp"

//TODO rng einfuegen und epsilon eingestellbar machen

typedef unsigned cluster_t;
typedef double precision_t;
typedef std::vector<precision_t> histogram_t;
typedef std::vector<histogram_t> histogram_c;

typedef struct {
  cluster_t cluster;
  histogram_t histogram;
} datapoint_t;

typedef std::vector<datapoint_t> dataset_t;

//typedef struct emd_dist {
  //std::vector<std::vector<precision_t>> cost_mat;

  //emd_dist(unsigned features) { gen_cost_matrix(features, features, cost_mat); }

  //precision_t distance(histogram_t &a, histogram_t &b, unsigned nb_elements) {
    //return emd_hat<precision_t>()(a, b, cost_mat);
  //}

//};

static void gen_cost_matrix(unsigned rows, unsigned cols,
                            std::vector<std::vector<precision_t>> &cost_mat) {
  cost_mat = std::vector<std::vector<precision_t>>(
      rows, std::vector<precision_t>(cols));

  int j = -1;
  for (unsigned int c1 = 0; c1 < rows; ++c1) {
    ++j;
    int i = -1;
    for (unsigned int c2 = 0; c2 < cols; ++c2) {
      ++i;
      cost_mat[i][j] = abs(i - j);
    }
  }
}

static precision_t emd_forwarder(histogram_t &a, histogram_t &b,
                                 unsigned nb_elements, void *context) {

  //static_cast<std::vector<std::vector<precision_t>> *>(context)->distance(a, b, nb_elements);
    return emd_hat<precision_t>()(a, b, *static_cast<std::vector<std::vector<precision_t>> *>(context));
}

static precision_t l2_distance(histogram_t &a, histogram_t &b,
                               unsigned nb_elements, void *context = NULL) {
  precision_t sum = 0;
  for (unsigned i = 0; i < nb_elements; ++i) {
    sum += (a[i] - b[i]) * (a[i] - b[i]);
  }
  return sqrt(sum);
}

static void kmeans_center_init_random(cluster_t nb_center, histogram_c &center,
                               dataset_t &dataset, nbgen &rng) {
  size_t nb_data = dataset.size();
  center.resize(nb_center);

  for (cluster_t i = 0; i < nb_center; i++)
    center[i] = dataset[rng() % nb_data].histogram;
}

static void kmeans_center_multiple_restarts(unsigned nb_restarts, cluster_t nb_center,
                                     void (*center_init_f)(cluster_t,
                                                           histogram_c &,
                                                           dataset_t &,nbgen &),
                                     histogram_c &center, dataset_t &dataset, nbgen &rng) {
  std::vector<histogram_c> center_c(nb_restarts);
  for (unsigned i = 0; i < nb_restarts; ++i)
    center_init_f(nb_center, center_c[i], dataset,rng);

  unsigned nb_features = dataset[0].histogram.size();
  std::vector<double> cluster_dists(nb_restarts);
  for (unsigned r = 0; r < nb_restarts; ++r) {
    double sum = 0;
    unsigned count = 0;
    std::vector<double> distances(nb_center, 0);
    for (unsigned i = 0; i < nb_center; ++i) {
      for (unsigned j = 0; j < nb_center; ++j) {
        if (j == i)
          continue;
        double dist = l2_distance(center_c[r][i], center_c[r][j], nb_features);
        distances[i] += dist;
        ++count;
      }
      sum += distances[i];
    }
    cluster_dists[r] = sum / count;
    // printf("restart:%u -> %f\n", r, cluster_dists[r]);
  }
  size_t max_cluster = std::distance(
      cluster_dists.begin(),
      std::max_element(cluster_dists.begin(), cluster_dists.end()));
  // printf("min center index: %zu\n", max_cluster);
  center = center_c[max_cluster];
}

// center have to be initialized before calling this function.
static void
kmeans(cluster_t nb_clusters, dataset_t &dataset,
       precision_t (*distFunc)(histogram_t &, histogram_t &, unsigned, void *),
       histogram_c &center, unsigned nb_threads = 1,
       precision_t epsilon = 0.01, void *context = NULL) {
  size_t nb_data, nb_features, accumulator, per_block, changed, iter;

  if (nb_clusters > dataset.size())
    return;

  iter = 0;
  nb_data = dataset.size();
  nb_features = dataset[0].histogram.size();
  per_block = nb_data / nb_threads;

  std::vector<size_t> thread_block_size(nb_threads, per_block);
  thread_block_size.back() += nb_data - nb_threads * per_block;
  std::vector<std::thread> eval_threads(nb_threads);

  do {
    changed = 0;
    accumulator = 0;

    for (int t = 0; t < nb_threads; ++t) {
      accumulator += thread_block_size[t];
      eval_threads[t] =
          std::thread([t, accumulator, &dataset, &thread_block_size,
                       &nb_clusters, &center, &changed, &distFunc, &nb_features,
                       &context] {
            cluster_t curr_cluster, min_cluster;
            for (size_t i = (accumulator - thread_block_size[t]);
                 i < accumulator; ++i) {
              curr_cluster = dataset[i].cluster;
              std::vector<precision_t> variance(nb_clusters);

              for (unsigned b = 0; b < nb_clusters; ++b)
                variance[b] = (*distFunc)(dataset[i].histogram, center[b],
                                          nb_features, context);

              min_cluster = std::distance(
                  variance.begin(),
                  std::min_element(variance.begin(), variance.end()));
              if (min_cluster != curr_cluster)
                ++changed;
              dataset[i].cluster = min_cluster;
            }
          });
    }

    for (int t = 0; t < nb_threads; ++t)
      eval_threads[t].join();

    std::vector<precision_t> cluster_element_counter(nb_clusters, 0);
    std::vector<std::vector<precision_t>> cluster_probability_mass(
        nb_clusters, histogram_t(nb_features, 0));
    for (unsigned i = 0; i < nb_data; ++i) {
      ++cluster_element_counter[dataset[i].cluster];
      for (unsigned j = 0; j < nb_features; ++j) {
        cluster_probability_mass[dataset[i].cluster][j] +=
            dataset[i].histogram[j];
      }
    }

    for (unsigned i = 0; i < nb_clusters; ++i) {
      for (unsigned j = 0; j < nb_features; ++j)
        if (cluster_probability_mass[i][j] > 0)
          cluster_probability_mass[i][j] /= cluster_element_counter[i];

      center[i] = cluster_probability_mass[i];
    }

    ++iter;
     printf("#%zu elements changed: %zu -> %f%%\n",iter, changed,
     100 * ((1.0 * changed) / nb_data));
  } while ((1.0 * changed) / nb_data > epsilon);
}

#endif
