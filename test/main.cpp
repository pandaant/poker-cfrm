#include <cstdio>
#include "../include/action_translation.hpp"

int main() {
    srand(1000);
    PseudoHarmonicMapping mapper;
    std::vector<double> abs{10,100,120, 500,1000};
    int hist[] = {0,0,0,0,0};
    unsigned runs = 100;
    for(unsigned i = 0; i < runs; ++i){
    ++hist[mapper.map_rand(abs,110)];
    //printf("result: %f\n",abs[mapper.map_rand(abs,110)]);
    }

    for(unsigned i = 0; i < abs.size(); ++i)
    printf("%u = %f\n",i, hist[i]/(runs+0.0));
  return 0;
}
