#include <math.h>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstddef>
#include <iterator>
#include <algorithm>

typedef unsigned cluster_t;
typedef float precision_t;
typedef std::vector<precision_t> histogram_t;
typedef std::vector<histogram_t> histogram_c;

typedef struct {
  cluster_t cluster;
  histogram_t histogram;
} datapoint_t;

typedef std::vector<datapoint_t> dataset_t;

static precision_t l2_distance(histogram_t &a, histogram_t &b,
                               unsigned nb_elements) {
  precision_t sum = 0;
  for (unsigned i = 0; i < nb_elements; ++i) {
    sum += (a[i] - b[i]) * (a[i] - b[i]);
  }
  return sqrt(sum);
}

void kmeans_center_init_random(cluster_t nb_center, histogram_c &center,
                               dataset_t &dataset) {
  size_t nb_data = dataset.size();
  center.resize(nb_center);

  for (cluster_t i = 0; i < nb_center; i++)
    center[i] = dataset[rand() % nb_data].histogram;
}

void kmeans_center_multiple_restarts(unsigned nb_restarts, cluster_t nb_center,
                                     void (*center_init_f)(cluster_t,
                                                                  histogram_c &,
                                                                  dataset_t &),
                                     histogram_c &center, dataset_t &dataset) {
   std::vector<histogram_c> center_c(nb_restarts);
   for (unsigned i = 0; i < nb_restarts; ++i)
     center_init_f(nb_center, center_c[i], dataset);

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
     //printf("restart:%u -> %f\n", r, cluster_dists[r]);
   }
   size_t max_cluster = std::distance(
       cluster_dists.begin(),
       std::max_element(cluster_dists.begin(), cluster_dists.end()));
   //printf("min center index: %zu\n", max_cluster);
   center = center_c[max_cluster];
}

void kmeans(cluster_t nb_clusters, dataset_t &dataset,
            precision_t (*distFunc)(histogram_t &, histogram_t &, unsigned),
            histogram_c &center, bool init_centers = true,
            unsigned nb_threads = 1, precision_t epsilon = 0.01) {
  size_t nb_data, nb_features, accumulator, per_block, changed, iter;

  if (nb_clusters > dataset.size())
    return;

  if (init_centers) {
      kmeans_center_init_random(nb_clusters, center, dataset);
  }

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
      eval_threads[t] = std::thread([t, accumulator, &dataset,
                                     &thread_block_size, &nb_clusters, &center,
                                     &changed, &distFunc, &nb_features] {
        cluster_t curr_cluster, min_cluster;
        for (size_t i = (accumulator - thread_block_size[t]); i < accumulator;
             ++i) {
          curr_cluster = dataset[i].cluster;
          std::vector<precision_t> variance(nb_clusters);

          for (unsigned b = 0; b < nb_clusters; ++b)
            variance[b] =
                (*distFunc)(dataset[i].histogram, center[b], nb_features);

          min_cluster =
              std::distance(variance.begin(),
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
    // printf("#%zu elements changed: %zu -> %f%%\n",iter, changed,
    // 100 * ((1.0 * changed) / nb_data));
  } while ((1.0 * changed) / nb_data > epsilon);
}

int main() {
  srand(100000);

  size_t clusters = 100;
  size_t nb_data = 1000000;
  size_t nb_features = 8;

  dataset_t dataset(nb_data);
  for (unsigned i = 0; i < nb_data; ++i) {
    dataset[i] = {0, histogram_t(nb_features)};
    size_t sum = 0;
    for (unsigned j = 0; j < nb_features; ++j) {
      dataset[i].histogram[j] = (precision_t)rand();
      sum += dataset[i].histogram[j];
    }

    for (unsigned j = 0; j < nb_features; ++j) {
      dataset[i].histogram[j] /= sum;
    }
  }

  // printf("%f\n",l2_distance(dataset[0].histogram, dataset[1].histogram,3));

  histogram_c center;
  unsigned restarts = 100;
  //kmeans_center_multiple_restarts(restarts, clusters, kmeans_center_init_random, center, dataset);
   kmeans(clusters, dataset, l2_distance, center, true,6,0.01);

   printf("centers:\n");
   for (size_t i = 0; i < clusters; ++i) {
   printf("%zu = ", i);
   for (unsigned f = 0; f < nb_features; ++f) {
   printf("%f ", center[i][f]);
  }
   printf("\n");
  }

  return 0;
}
