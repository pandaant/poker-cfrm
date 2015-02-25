#ifndef ACTION_ABSTRACTION_HPP
#define ACTION_ABSTRACTION_HPP

#include <string>
#include <fstream>

using std::string;

class ActionAbstraction {
public:
  virtual ~ActionAbstraction();
  virtual action_c get_actions(const Game *game, const State &state) = 0;
};

class NullActionAbstraction : public ActionAbstraction {
public:
  NullActionAbstraction(const Game *game, string param) {}
  virtual action_c get_actions(const Game *game, const State &state);
};

//TODO allin an aktuellen stack anpassen und nicht mit 9999 "simulieren" das gibt probleme mit der action translation function
class PotRelationActionAbstraction : public ActionAbstraction {
  const Game *game;
  std::vector<double> fractions;

public:
  PotRelationActionAbstraction(const Game *game, std::vector<double> fractions);
  virtual action_c get_actions(const Game *game, const State &state);
};

#endif
