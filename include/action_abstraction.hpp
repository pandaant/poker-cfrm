#ifndef ACTION_ABSTRACTION_HPP
#define ACTION_ABSTRACTION_HPP

#include <string>
#include <fstream>

using std::string;

class ActionAbstraction {
public:
  virtual ~ActionAbstraction();
  virtual action_c get_actions(const Game *game, const State &state) = 0;
  virtual void dump(std::string filename) {}
};

class NullActionAbstraction : public ActionAbstraction {
public:
  NullActionAbstraction(const Game *game, string param) {}
  virtual action_c get_actions(const Game *game, const State &state);
  virtual void dump(std::string filename) {}
};

//TODO allin an aktuellen stack anpassen und nicht mit 9999 "simulieren" das gibt probleme mit der action translation function
class PotRelationActionAbstraction : public ActionAbstraction {
  const Game *game;
  std::vector<double> fractions;

public:
  PotRelationActionAbstraction(const Game *game, std::vector<double> fractions);
  virtual action_c get_actions(const Game *game, const State &state);

  virtual void dump(std::string filename) {
      string f = filename + ".aa";
    std::ofstream fs(f, std::ios::out | std::ios::binary);
    size_t nb_fractions = fractions.size(); 
    fs.write(reinterpret_cast<const char *>(&nb_fractions), sizeof(nb_fractions));
    for (unsigned i = 0; i < nb_fractions; ++i) {
      fs.write(reinterpret_cast<const char *>(&fractions[i]),
               sizeof(fractions[i]));
    }
    fs.close();
  }
};

#endif
