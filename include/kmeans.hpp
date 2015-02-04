#ifndef KMEANS_HPP
#define KMEANS_HPP

#define NULL 0

#include <vector>
#include <cstdlib>
#include <math.h>
#include <algorithm>
#include <boost/static_assert.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "emd_hat.hpp"
#include "definitions.hpp"

/*
EXAMPLE FOR BASIC DATAPOINT STRUCTURE:

struct datapoint {
  double value;
  unsigned cluster;
};
*/

static double l2_distance(dbl_c &hist_a, dbl_c &hist_b, int threads = 1){
    double sum = 0;
    for(unsigned i = 0; i < hist_a.size(); ++i){
        sum += (hist_a[i] - hist_b[i]) * (hist_a[i] - hist_b[i]);
    }
   return sum; 
}

static std::vector<dbl_c> gen_cost_matrix(unsigned rows, unsigned cols) {
  std::vector<dbl_c> cost_mat(rows, dbl_c(cols));

  int max_cost_mat = -1;
  int j = -1;
  for (unsigned int c1 = 0; c1 < rows; ++c1) {
    ++j;
    int i = -1;
    for (unsigned int c2 = 0; c2 < cols; ++c2) {
      ++i;
      cost_mat[i][j] = abs(i - j);
    }
  }
  return cost_mat;
}

static double emd_distance(dbl_c &hist_a, dbl_c &hist_b,
                           std::vector<dbl_c> cost_mat) {
  return emd_hat<double>()(hist_a, hist_b, cost_mat);
}

// TODO init rng externally!
template <class DATAPOINT>
void kmpp(std::vector<double> &centers, std::vector<DATAPOINT> &dataset) {
  boost::mt19937 rng(time(0));
  centers[0] = dataset[rand() % dataset.size()].value;
  for (unsigned i = 1; i < centers.size(); ++i) {
    std::vector<double> variance(
        dataset.size()); // TODO das aus dem loop nehmen?
    for (unsigned d = 0; d < dataset.size(); ++d) {
      variance[d] = fabs(centers[i - 1] - dataset[d].value);
      variance[d] *= variance[d];
    }

    boost::random::discrete_distribution<> dist(variance.begin(),
                                                variance.end());
    centers[i] = dataset[dist(rng)].value;
  }
  // centers[i] = dataset[rand() % dataset.size()].value;
}

template <class DATAPOINT>
void kmpp_l2(std::vector<DATAPOINT> &centers, std::vector<DATAPOINT> &dataset,
             unsigned nb_threads = 1) {
  std::cout << "initializing clustering with kmeans++ ...";
  boost::mt19937 rng(time(0));

  size_t num_data = dataset.size();
  size_t num_features = dataset[0].histogram.size();

  int per_block = num_data / nb_threads;
  std::vector<size_t> thread_block_size(nb_threads, per_block);
  thread_block_size.back() += num_data - nb_threads * per_block;
  std::vector<std::thread> eval_threads(nb_threads);
  size_t accumulator;

  centers[0] = dataset[rand() % num_data];
  for (unsigned curr_center = 1; curr_center < centers.size(); ++curr_center) {
    std::vector<double> variance(num_data); // TODO das aus dem loop nehmen?
    accumulator = 0;

    for (int t = 0; t < nb_threads; ++t) {
      accumulator += thread_block_size[t];
      eval_threads[t] =
          std::thread([t, accumulator, &dataset, &thread_block_size,
                      &centers, &rng, &curr_center, &num_data, &variance] {
            for (size_t i = (accumulator - thread_block_size[t]);
                 i < accumulator; ++i) {
              if (t == 0 && i % 10000 == 0)
                std::cout << "\r" << (int)(100 * (i / (1.0 * accumulator)))
                          << "%" << std::flush;

                variance[i] = l2_distance(centers[curr_center - 1].histogram,
                                           dataset[i].histogram);
                variance[i] *= variance[i];
            }
          });
    }

    for (int t = 0; t < nb_threads; ++t)
      eval_threads[t].join();
    std::cout << "\r100%\n";
  std::cout << "done.\n";

    boost::random::discrete_distribution<> dist(variance.begin(),
                                                variance.end());
    size_t choosen = dist(rng);
    centers[curr_center] = dataset[choosen];
    //std::cout << "center " << curr_center << " has distance " << fabs(variance[choosen])
              //<< "\n";
  }
  // centers[i] = dataset[rand() % dataset.size()].value;
}

template <class DATAPOINT>
void kmpp_emd(std::vector<DATAPOINT> &centers, std::vector<DATAPOINT> &dataset,
              std::vector<dbl_c> cost_mat, unsigned nb_threads = 1) {
  std::cout << "initializing clustering with kmeans++ ...";
  boost::mt19937 rng(time(0));

  size_t num_data = dataset.size();
  size_t num_features = dataset[0].histogram.size();

  int per_block = num_data / nb_threads;
  std::vector<size_t> thread_block_size(nb_threads, per_block);
  thread_block_size.back() += num_data - nb_threads * per_block;
  std::vector<std::thread> eval_threads(nb_threads);
  size_t accumulator;

  centers[0] = dataset[rand() % num_data];
  for (unsigned curr_center = 1; curr_center < centers.size(); ++curr_center) {
    std::vector<double> variance(num_data); // TODO das aus dem loop nehmen?
    accumulator = 0;

    for (int t = 0; t < nb_threads; ++t) {
      accumulator += thread_block_size[t];
      eval_threads[t] =
          std::thread([t, accumulator, &dataset, &thread_block_size,
                      &centers, &cost_mat, &rng, &curr_center, &num_data, &variance] {
            for (size_t i = (accumulator - thread_block_size[t]);
                 i < accumulator; ++i) {
              if (t == 0 && i % 10000 == 0)
                std::cout << "\r" << (int)(100 * (i / (1.0 * accumulator)))
                          << "%" << std::flush;

                variance[i] = emd_distance(centers[curr_center - 1].histogram,
                                           dataset[i].histogram, cost_mat);
                variance[i] *= variance[i];
            }
          });
    }

    for (int t = 0; t < nb_threads; ++t)
      eval_threads[t].join();
    std::cout << "\r100%\n";
  std::cout << "done.\n";

    boost::random::discrete_distribution<> dist(variance.begin(),
                                                variance.end());
    size_t choosen = dist(rng);
    centers[curr_center] = dataset[choosen];
    //std::cout << "center " << curr_center << " has distance " << fabs(variance[choosen])
              //<< "\n";
  }
  // centers[i] = dataset[rand() % dataset.size()].value;
}

// integrate multithreading TODO
template <class DATAPOINT>
void kmeans(unsigned nb_clusters, std::vector<DATAPOINT> &dataset,
            double epsilon = 0.01) {
  using std::vector;
  vector<double> centers(nb_clusters);
  kmpp(centers, dataset);

  unsigned changed;
  do {
    changed = 0;
    for (unsigned i = 0; i < dataset.size(); ++i) {
      vector<double> variance(nb_clusters);
      unsigned curr_cluster = dataset[i].cluster;
      for (unsigned b = 0; b < nb_clusters; ++b) {
        variance[b] = fabs(dataset[i].value - centers[b]);
      }

      size_t min_cluster = std::distance(
          variance.begin(), std::min_element(variance.begin(), variance.end()));
      if (min_cluster != curr_cluster)
        ++changed;
      dataset[i].cluster = min_cluster;
    }

    vector<double> cluster_element_counter(nb_clusters, 0);
    vector<double> cluster_element_sums(nb_clusters, 0);
    for (unsigned i = 0; i < dataset.size(); ++i) {
      cluster_element_sums[dataset[i].cluster] += dataset[i].value;
      ++cluster_element_counter[dataset[i].cluster];
    }
    for (unsigned b = 0; b < nb_clusters; ++b) {
      centers[b] = (1.0 / cluster_element_counter[b]) * cluster_element_sums[b];
    }
  } while ((1.0 * changed) / dataset.size() > epsilon);
}

template <class DATAPOINT>
void kmeans_emd(unsigned nb_clusters, std::vector<DATAPOINT> &dataset,
                unsigned nb_threads = 1, double epsilon = 0.01) {
  using std::vector;

  if (dataset.empty())
    return;

  vector<DATAPOINT> centers(nb_clusters);
  std::vector<dbl_c> cost_mat =
      gen_cost_matrix(dataset[0].histogram.size(), dataset[0].histogram.size());
  kmpp_emd(centers, dataset, cost_mat,nb_threads);

  size_t num_data = dataset.size();
  size_t num_features = dataset[0].histogram.size();
  //for (unsigned i = 0; i < nb_clusters; ++i)
    //centers[i] = num_data > i ? dataset[i] : DATAPOINT();

  int per_block = num_data / nb_threads;
  std::vector<size_t> thread_block_size(nb_threads, per_block);
  thread_block_size.back() += num_data - nb_threads * per_block;
  std::vector<std::thread> eval_threads(nb_threads);
  size_t accumulator;

  unsigned changed;
  do {
    changed = 0;
    accumulator = 0;

    for (int t = 0; t < nb_threads; ++t) {
      accumulator += thread_block_size[t];
      // std::cout << "i am thread " << t << "\n";
      // (t*thread_block_size[t]) << " to " << accumulator << "\n";
      eval_threads[t] =
          std::thread([t, accumulator, &dataset, &thread_block_size,
                       &nb_clusters, &centers, &cost_mat, &changed] {
            for (size_t i = (accumulator - thread_block_size[t]);
                 i < accumulator; ++i) {
              if (t == 0 && i % 10000 == 0)
                std::cout << "\r" << (int)(100 * (i / (1.0 * accumulator)))
                          << "%" << std::flush;
              vector<double> variance(nb_clusters);
              unsigned curr_cluster = dataset[i].cluster;
              for (unsigned b = 0; b < nb_clusters; ++b) {
                // variance[b] = fabs(emd_distance(
                // dataset[i].histogram, centers[b].histogram, cost_mat));
                variance[b] = emd_distance(dataset[i].histogram,
                                           centers[b].histogram, cost_mat);
              }

              size_t min_cluster = std::distance(
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
    std::cout << "\r100%\n";

    // assign new center. element that has smallest mean distance to other
    // elements in bucket
    vector<double> cluster_element_counter(nb_clusters, 0);
    vector<vector<double>> cluster_probability_mass(nb_clusters,
                                                    dbl_c(num_features, 0));
    vector<double> cluster_total_probability_mass(nb_clusters, 0);
    for (unsigned i = 0; i < num_data; ++i) {
      ++cluster_element_counter[dataset[i].cluster];
      for (unsigned j = 0; j < num_features; ++j) {
        cluster_probability_mass[dataset[i].cluster][j] +=
            dataset[i].histogram[j];
        cluster_total_probability_mass[dataset[i].cluster] +=
            dataset[i].histogram[j]; // should always be 1
      }
    }

    // calculate normalized new center from cluster mass
    for (unsigned i = 0; i < nb_clusters; ++i) {
      // std::cout << "total mass of cluster " << i << ": " <<
      // cluster_total_probability_mass[i] << "\n";
      for (unsigned j = 0; j < num_features; ++j) {
        // std::cout << "feature " << j << ": " <<
        // cluster_probability_mass[i][j] << ", after: ";
        if (cluster_total_probability_mass[i] > 0)
          cluster_probability_mass[i][j] /= cluster_total_probability_mass[i];
        // std::cout <<cluster_probability_mass[i][j] << "\n";
      }

      // assign new center to bucket i
      centers[i] = DATAPOINT();
      centers[i].histogram = cluster_probability_mass[i];
    }

    std::cout << "changed: " << changed << "\n";
  } while ((1.0 * changed) / dataset.size() > epsilon);
}

template <class DATAPOINT>
void kmeans_l2(unsigned nb_clusters, std::vector<DATAPOINT> &dataset,
                unsigned nb_threads = 1, double epsilon = 0.01) {
  using std::vector;

  if (dataset.empty())
    return;

  vector<DATAPOINT> centers(nb_clusters);
  //std::vector<dbl_c> cost_mat =
      //gen_cost_matrix(dataset[0].histogram.size(), dataset[0].histogram.size());
  kmpp_l2(centers, dataset, nb_threads);

  size_t num_data = dataset.size();
  size_t num_features = dataset[0].histogram.size();
  //for (unsigned i = 0; i < nb_clusters; ++i)
    //centers[i] = num_data > i ? dataset[i] : DATAPOINT();

  int per_block = num_data / nb_threads;
  std::vector<size_t> thread_block_size(nb_threads, per_block);
  thread_block_size.back() += num_data - nb_threads * per_block;
  std::vector<std::thread> eval_threads(nb_threads);
  size_t accumulator;

  unsigned changed;
  do {
    changed = 0;
    accumulator = 0;

    for (int t = 0; t < nb_threads; ++t) {
      accumulator += thread_block_size[t];
      // std::cout << "i am thread " << t << "\n";
      // (t*thread_block_size[t]) << " to " << accumulator << "\n";
      eval_threads[t] =
          std::thread([t, accumulator, &dataset, &thread_block_size,
                       &nb_clusters, &centers, &changed] {
            for (size_t i = (accumulator - thread_block_size[t]);
                 i < accumulator; ++i) {
              if (t == 0 && i % 10000 == 0)
                std::cout << "\r" << (int)(100 * (i / (1.0 * accumulator)))
                          << "%" << std::flush;

              vector<double> variance(nb_clusters);
              unsigned curr_cluster = dataset[i].cluster;
              for (unsigned b = 0; b < nb_clusters; ++b) {
                variance[b] = l2_distance(dataset[i].histogram,
                                           centers[b].histogram);
              }

              size_t min_cluster = std::distance(
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
    std::cout << "\r100%\n";

    // assign new center. element that has smallest mean distance to other
    // elements in bucket
    vector<double> cluster_element_counter(nb_clusters, 0);
    vector<vector<double>> cluster_probability_mass(nb_clusters,
                                                    dbl_c(num_features, 0));
    vector<double> cluster_total_probability_mass(nb_clusters, 0);
    for (unsigned i = 0; i < num_data; ++i) {
      ++cluster_element_counter[dataset[i].cluster];
      for (unsigned j = 0; j < num_features; ++j) {
        cluster_probability_mass[dataset[i].cluster][j] +=
            dataset[i].histogram[j];
        cluster_total_probability_mass[dataset[i].cluster] +=
            dataset[i].histogram[j]; // should always be 1
      }
    }

    // calculate normalized new center from cluster mass
    for (unsigned i = 0; i < nb_clusters; ++i) {
      for (unsigned j = 0; j < num_features; ++j) {
        if (cluster_total_probability_mass[i] > 0)
          cluster_probability_mass[i][j] /= cluster_total_probability_mass[i];
      }

      // assign new center to bucket i
      centers[i] = DATAPOINT();
      centers[i].histogram = cluster_probability_mass[i];
    }
    std::cout << "changed: " << changed << "\n";
  } while ((1.0 * changed) / dataset.size() > epsilon);
}

#endif
