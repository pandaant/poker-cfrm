#ifndef MAIN_FUNCTIONS_HPP
#define MAIN_FUNCTIONS_HPP

#include <cstdlib>
#include <string>
#include "card_abstraction.hpp"
#include "action_abstraction.hpp"

std::vector<std::string> &split(const std::string &s, char delim,
                                std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}

std::vector<int> str_to_int_list(const std::string &s, char delim) {
  std::vector<std::string> elems = split(s, delim);
  std::vector<int> us(elems.size());
  for(unsigned i = 0; i < elems.size(); ++i)
      us[i] = atoi(elems[i].c_str());
  return us;
}

std::vector<double> str_to_dbl_list(const std::string &s, char delim) {
  std::vector<std::string> elems = split(s, delim);
  std::vector<double> dbls(elems.size());
  for(unsigned i = 0; i < elems.size(); ++i)
      dbls[i] = atof(elems[i].c_str());
  return dbls;
}

CardAbstraction *load_card_abstraction(const Game *gamedef,
                                       card_abstraction abs, string param) {
  switch (abs) {
  case NULLCARD_ABS:
    return new NullCardAbstraction(gamedef, param);
    break;
  case BLINDCARD_ABS:
    return new BlindCardAbstraction(gamedef, param);
    break;
  case CLUSTERCARD_ABS:
    return new ClusterCardAbstraction(gamedef, param);
    break;
  };
  throw std::runtime_error("unknown card abstraction");
}

ActionAbstraction *load_action_abstraction(const Game *gamedef,
                                           action_abstraction abs,
                                           string param) {
  switch (abs) {
  case NULLACTION_ABS:
    return new NullActionAbstraction(gamedef, param);
    break;
  case POTRELACTION_ABS:
    if(param == ""){
        printf("missing parameter \"fractions\" for pot relation action abstraction\n");
        exit(1);
    }

    std::vector<double> fractions = str_to_dbl_list(param,',');
    //for(unsigned i = 0; i < fractions.size(); ++i)
        //std::cout << fractions[i] << "\n";
    return new PotRelationActionAbstraction(gamedef, fractions);
    break;
  };
  throw std::runtime_error("unknown action abstraction");
}

#endif
