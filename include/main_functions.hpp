#ifndef MAIN_FUNCTIONS_HPP
#define MAIN_FUNCTIONS_HPP

CardAbstraction *load_card_abstraction(const Game* gamedef, card_abstraction abs, string param){
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
    return new NullActionAbstraction(gamedef,param);
    break;
  case POTRELACTION_ABS:
    return new PotRelationActionAbstraction(gamedef,std::vector<double>{0.75,1,2,9999});
    break;
  };
  throw std::runtime_error("unknown action abstraction");
}

#endif
