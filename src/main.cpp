#include <stdio.h>
#include <memory>
#include <random>
#include <vector>
#include <unordered_map>
#include <deque>

#include "core.h"
#include "decision_engine.h"
#include "action_factories.h"
#include "sarsa_agent.h"
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
  CompositeActionFactory cf({
                             {"WorkAction", &waf},
                             {"GiveAction", &gaf},
                             {"AskAction", &aaf},
                             {"TrivialAction", &taf}});
  AskResponseFactory rf;

  ProbDistPolicy pdp;

  int num_heuristic_agents = 5;

  for (int i = 0; i < num_heuristic_agents; i++) {
    c.push_back(std::make_unique<Character>(i, money_dist(random_generator)));
    characters.push_back(c.back().get());
    characters.back()->traits_[kBackground] = background_dist(random_generator);
    characters.back()->traits_[kLanguage] = language_dist(random_generator);

    a.push_back(
        std::make_unique<HeuristicAgent>(characters.back(), &cf, &rf, &pdp));
    agents.push_back(a.back().get());
  }

  double e = 0.05;
  double n = 0.001;
  double b1 = 0.9;
  double b2 = 0.999;
  double g = 0.8;
  int n_steps = 100;
  int num_learning_agents = 5;

  //learning agents
  FILE* learn_log = fopen("/tmp/learn_log", "a");
  setvbuf(learn_log, NULL, _IOLBF, 1024*10);
  Logger give_learn_logger("learn_give", learn_log);
  Logger ask_learn_logger("learn_ask", learn_log);
  Logger ask_success_learn_logger("learn_ask_success", learn_log);
  Logger ask_failure_learn_logger("learn_ask_failure", learn_log);
  Logger trivial_learn_logger("learn_trivial", learn_log);
  Logger work_learn_logger("learn_work", learn_log);

  FILE* policy_log = fopen("/tmp/policy_log", "a");
  setvbuf(policy_log, NULL, _IOLBF, 1024*10);
  Logger policy_logger("policy", policy_log);

  std::unique_ptr<SARSAGiveActionFactory> sgaf =
      SARSAGiveActionFactory::Create(SARSALearner::Create(
          n, g, b1, b2, random_generator, 10, &give_learn_logger));
  std::unique_ptr<SARSAAskActionFactory> saaf =
      SARSAAskActionFactory::Create(SARSALearner::Create(
          n, g, b1, b2, random_generator, 10, &ask_learn_logger));
  std::unique_ptr<SARSATrivialActionFactory> staf =
      SARSATrivialActionFactory::Create(SARSALearner::Create(
          n, g, b1, b2, random_generator, 6, &trivial_learn_logger));
  std::unique_ptr<SARSAWorkActionFactory> swaf =
      SARSAWorkActionFactory::Create(SARSALearner::Create(
          n, g, b1, b2, random_generator, 6, &work_learn_logger));

  std::vector<SARSAActionFactory*> sarsa_action_factories({
      sgaf.get(), saaf.get(), swaf.get(), staf.get()});
  /*std::vector<SARSAActionFactory*> sarsa_action_factories({
      staf.get(), swaf.get()});*/

  std::unique_ptr<SARSAAskSuccessResponseFactory> asrf =
      SARSAAskSuccessResponseFactory::Create(SARSALearner::Create(
          n, g, b1, b2, random_generator, 10, &ask_success_learn_logger));
  std::unique_ptr<SARSAAskFailureResponseFactory> afrf =
      SARSAAskFailureResponseFactory::Create(SARSALearner::Create(
          n, g, b1, b2, random_generator, 10, &ask_failure_learn_logger));

  std::unordered_map<std::string, std::set<SARSAResponseFactory*>>
      sarsa_response_factories({{"AskAction", {asrf.get(), afrf.get()}}});

  //scf.ReadWeights();
  EpsilonGreedyPolicy egp(e, &policy_logger);
  int num_non_learning_agents = agents.size();
  for (int i = 0; i < num_learning_agents; i++) {
    c.push_back(std::make_unique<Character>(i + num_non_learning_agents,
                                            money_dist(random_generator)));
    characters.push_back(c.back().get());
    characters.back()->traits_[kBackground] = background_dist(random_generator);
    characters.back()->traits_[kLanguage] = language_dist(random_generator);

    a.push_back(
        std::make_unique<SARSAAgent>(characters.back(), sarsa_action_factories,
                                     sarsa_response_factories, &egp, n_steps));
    agents.push_back(a.back().get());
  }

  FILE* action_log = fopen("/tmp/action_log", "a");
  setvbuf(action_log, NULL, _IOLBF, 1024*10);
  Logger action_logger = Logger("action", action_log);
  logger.Log(INFO, "creating CVC\n");
  CVC cvc(characters, &logger, random_generator);

  std::unique_ptr<DecisionEngine> d =
      DecisionEngine::Create(agents, &cvc, &action_logger);

  logger.Log(INFO, "running the game loop\n");

  cvc.LogState();
  for (; cvc.Now() < 100000;) {
    d->RunOneGameLoop();

    if(cvc.Now() % 10000 == 0) {
      cvc.LogState();
    }
  }
  cvc.LogState();

  //scf.WriteWeights();

  fclose(action_log);
  fclose(learn_log);
  fclose(policy_log);

  return 0;
}
