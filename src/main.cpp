#include <stdio.h>
#include <memory>
#include <random>
#include <vector>
#include <unordered_map>

#include "core.h"
#include "decision_engine.h"
#include "action_factories.h"
#include "sarsa_action_factories.h"

int main(int argc, char** argv) {
  Logger logger;
  logger.Log(INFO, "Setting up Characters\n");

  std::random_device rd;
  std::mt19937 random_generator(rd());

  std::vector<std::unique_ptr<Character>> c;
  std::vector<Character*> characters;
  std::uniform_real_distribution<> money_dist(10.0, 25.0);
  std::uniform_int_distribution<> background_dist(0, 10);
  std::uniform_int_distribution<> language_dist(0, 5);

  std::vector<std::unique_ptr<Agent>> a;
  std::vector<Agent*> agents;

  //heuristic agents
  GiveActionFactory gaf;
  AskActionFactory aaf;
  WorkActionFactory waf;
  TrivialActionFactory taf;
  CompositeActionFactory cf({{"GiveAction", &gaf},
                             {"AskAction", &aaf},
                             {"WorkAction", &waf},
                             {"TrivialAction", &taf}});
  ProbDistPolicy pdp;
  for(int i=0; i<5; i++) {
    c.push_back(std::make_unique<Character>(i, money_dist(random_generator)));
    characters.push_back(c.back().get());
    characters.back()->traits_[kBackground] = background_dist(random_generator);
    characters.back()->traits_[kLanguage] = language_dist(random_generator);

    a.push_back(std::make_unique<Agent>(characters.back(), &cf, &pdp));
    agents.push_back(a.back().get());
  }

  //learning agents
  std::unique_ptr<SARSAGiveActionFactory> sgaf =
      SARSAGiveActionFactory::Create(random_generator);
  std::unique_ptr<SARSAAskActionFactory> saaf =
      SARSAAskActionFactory::Create(random_generator);
  std::unique_ptr<SARSATrivialActionFactory> staf =
      SARSATrivialActionFactory::Create(random_generator);
  std::unique_ptr<SARSAWorkActionFactory> swaf =
      SARSAWorkActionFactory::Create(random_generator);
  CompositeActionFactory scf({{"GiveAction", sgaf.get()},
                              {"AskAction", saaf.get()},
                              {"WorkAction", swaf.get()},
                              {"TrivialAction", staf.get()}});
  EpsilonGreedyPolicy egp(0.2);
  int num_learning_agents = agents.size();
  for(int i=0; i<5; i++) {
    c.push_back(std::make_unique<Character>(i + num_learning_agents,
                                            money_dist(random_generator)));
    characters.push_back(c.back().get());
    characters.back()->traits_[kBackground] = background_dist(random_generator);
    characters.back()->traits_[kLanguage] = language_dist(random_generator);

    a.push_back(std::make_unique<Agent>(characters.back(), &scf, &egp));
    agents.push_back(a.back().get());
  }

  FILE* action_log = fopen("/tmp/action_log", "a");
  logger.Log(INFO, "creating CVC\n");
  CVC cvc(characters, &logger, random_generator);

  std::unique_ptr<DecisionEngine> d =
      DecisionEngine::Create(agents, &cvc, action_log);

  logger.Log(INFO, "running the game loop\n");
  d->GameLoop();

  fclose(action_log);

  return 0;
}
