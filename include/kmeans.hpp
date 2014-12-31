#ifndef KMEANS_HPP
#define KMEANS_HPP

#include <vector>
#include <cstdlib>
#include <math.h>
#include <algorithm>
#include <boost/static_assert.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>

/*
EXAMPLE FOR BASIC DATAPOINT STRUCTURE:

struct datapoint {
  double value;
  unsigned cluster;
};
*/

template <class DATAPOINT>
void kmpp(std::vector<double> &centers, std::vector<DATAPOINT> &dataset){
    boost::mt19937 rng( time( 0 ) );
    centers[0] = dataset[rand() % dataset.size()].value;
  for (unsigned i = 1; i < centers.size(); ++i){
      std::vector<double> variance(dataset.size());// TODO das aus dem loop nehmen?
    for (unsigned d = 0; d < dataset.size(); ++d) {
        variance[d] = fabs(centers[i-1]-dataset[d].value);
        variance[d] *= variance[d];
    }

    boost::random::discrete_distribution<> dist(variance.begin(),variance.end());
    centers[i] = dataset[dist(rng)].value;
    
  }
    //centers[i] = dataset[rand() % dataset.size()].value;
}

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

#endif
