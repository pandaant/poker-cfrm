#ifndef ACTION_ABSTRACTION_HPP
#define ACTION_ABSTRACTION_HPP

#include <string>

using std::string;

class ActionAbstraction {
public:
  virtual ~ActionAbstraction();
  virtual action_c get_actions(const Game *game, const State &state) = 0;
};

class NullActionAbstraction : public ActionAbstraction {
    public:
    NullActionAbstraction(const Game* game, string param){}
  virtual action_c get_actions(const Game *game, const State &state);
};

#endif
