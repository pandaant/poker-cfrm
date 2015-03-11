#ifndef ACTION_TRANSLATION_HPP
#define ACTION_TRANSLATION_HPP

#include <vector>
#include <random>
#include <assert.h>

class ActionTranslation {
public:
  // returns the index for abs_actions that is best suited
  //virtual size_t map_det(std::vector<double> &abs_actions, double x) = 0;
  virtual size_t map_rand(std::vector<double> &abs_actions, double x) = 0;
};

class PseudoHarmonicMapping : public ActionTranslation {

public:
  // @return -1 if x < lowest bound -> map to index 0,
  //         1 if x > highest bound -> map to last index,
  //         0 if lower_bound and upper_bound are filled and returned.
  int get_bounds(std::vector<double> &abs_actions, double x,
                  unsigned &lower_bound, unsigned &upper_bound) {
    unsigned num_actions = abs_actions.size();

    if (x <= abs_actions[0])
      return -1;
    if (x >= abs_actions[num_actions - 1])
      return 1; 

    for (unsigned i = 0; i < num_actions; ++i) {
      if (abs_actions[i] <= x)
        lower_bound = i;
      if (abs_actions[i] >= x) {
        upper_bound = i;
        break;
      }
    }
      //printf("lower: %u, upper: %u\n",lower_bound, upper_bound);

    return 0;
  }

  virtual size_t map_rand(std::vector<double> &abs_actions, double x) {
    double a, b, x_median, f_ab_x;
    unsigned lower_bound, upper_bound, num_actions;
    num_actions = abs_actions.size();

      int res_type = get_bounds(abs_actions,x,lower_bound,upper_bound);

      if (res_type == -1)
        return 0;
      else if (res_type == 1)
        return num_actions - 1;

      a = abs_actions[lower_bound];
      b = abs_actions[upper_bound];
      f_ab_x = ((b-x)*(1+a)) / ((b-a)*(1+x));
      x_median = (a+b+2*a*b) / (a+b+2);

      //printf("fab(x)= %f\n",f_ab_x);
      //printf("median: %f\n",x_median);
      //printf("a/x* = %f, x*/b = %f\n",a/x_median,x_median/b);
      //printf("f/x* = %f, f*/b = %f\n",a/f_ab_x,f_ab_x/b);

      double weights[] = {f_ab_x, 1-f_ab_x};
      double dart =((double)rand())/RAND_MAX; 
      //printf("dart = %f\n",dart);
      unsigned choice;
      for(choice = 0; choice < 2; ++choice){
        if( dart < weights[choice] )
            break;
        dart -= weights[choice];
      //printf("dart = %f\n",dart);
      }
      //printf("choice = %f\n",weights[choice]);
      //assert(choice < 2);
      //assert(weights[choice] > 0.0);
      //if( (a/x_median) > (x_median/b))
          //return lower_bound;
      //return upper_bound;

      //printf("lower: %u, upper: %u\n",lower_bound, upper_bound);

      //double r = ((b-x)*(1+a)) / ((b-a)*(1+x));
      //printf("r: %f\n",r);

      //printf("threshold x*: %f\n",x_median);

      return choice == 0 ? lower_bound : upper_bound;
      
  }
};

#endif
