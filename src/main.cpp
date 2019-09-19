#include <stdio.h>
#include <memory>
#include <random>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>

#include "core.h"
#include "decision_engine.h"
#include "action_factories.h"
#include "sarsa/sarsa_agent.h"
#include "sarsa/sarsa_learner.h"
#include "sarsa/sarsa_action_factories.h"

class ActionsFactory {
 public:
  ActionsFactory(double n, double g, double b1, double b2,
                 std::mt19937* random_generator, Logger* learn_logger)
      : n_(n),
        g_(g),
        b1_(b1),
        b2_(b2),
        random_generator_(random_generator),
        learn_logger_(learn_logger) {}

  template <class AF>
  AF CreateFactory() {
    return AF(AF::CreateLearner(num_learners_++, n_, g_, b1_, b2_,
                                random_generator_, learn_logger_));
  }
 private:
  int num_learners_ = 0;
  double n_;
  double g_;
  double b1_;
  double b2_;
  std::mt19937* random_generator_;
  Logger* learn_logger_;
};


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

  int num_heuristic_agents = 0;

  for (int i = 0; i < num_heuristic_agents; i++) {
    c.push_back(std::make_unique<Character>(i, money_dist(random_generator)));
    characters.push_back(c.back().get());
    characters.back()->traits_[kBackground] = background_dist(random_generator);
    characters.back()->traits_[kLanguage] = language_dist(random_generator);

    a.push_back(
        std::make_unique<HeuristicAgent>(characters.back(), &cf, &rf, &pdp));
    agents.push_back(a.back().get());
  }

  //learning agents

  //double policy_greedy_e = 0.05;
  double policy_greedy_initial_e = 0.5;
  double policy_greedy_scale = 0.1;
  //double policy_temperature = 0.2;
  //double policy_initial_temperature = 50.0;
  //double policy_decay = 0.001;
  //double policy_scale = 0.001;
  double n = 0.001;
  double b1 = 0.9;
  double b2 = 0.999;
  double g = 0.9;
  int n_steps = 100;
  int num_learning_agents = 25;

  //learning agents
  FILE* learn_log = fopen("/tmp/learn_log", "a");
  setvbuf(learn_log, NULL, _IOLBF, 1024*10);
  Logger learn_logger("learner", learn_log, INFO);

  FILE* policy_log = fopen("/tmp/policy_log", "a");
  setvbuf(policy_log, NULL, _IOLBF, 1024*10);
  Logger policy_logger("policy", policy_log, WARN);

  ActionsFactory f(n, g, b1, b2, &random_generator, &learn_logger);
  auto sgaf = f.CreateFactory<cvc::sarsa::SARSAGiveActionFactory>();
  auto saaf = f.CreateFactory<cvc::sarsa::SARSAAskActionFactory>();
  auto staf = f.CreateFactory<cvc::sarsa::SARSATrivialActionFactory>();
  auto swaf = f.CreateFactory<cvc::sarsa::SARSAWorkActionFactory>();

  std::vector<cvc::sarsa::ActionFactory*> sarsa_action_factories(
      {&sgaf, &saaf, &swaf, &staf});

  auto asrf = f.CreateFactory<cvc::sarsa::SARSAAskSuccessResponseFactory>();
  auto afrf = f.CreateFactory<cvc::sarsa::SARSAAskFailureResponseFactory>();

  std::unordered_map<std::string, std::set<cvc::sarsa::ResponseFactory*>>
      sarsa_response_factories({{"AskAction", {&asrf, &afrf}}});

  //scf.ReadWeights();
  //cvc::sarsa::EpsilonGreedyPolicy learning_policy(policy_greedy_e, &policy_logger);
  cvc::sarsa::DecayingEpsilonGreedyPolicy learning_policy(policy_greedy_initial_e, policy_greedy_scale, &policy_logger);
  //cvc::sarsa::SoftmaxPolicy learning_policy(policy_temperature, &policy_logger);
  /*cvc::sarsa::GradSensitiveSoftmaxPolicy learning_policy(
      policy_initial_temperature, policy_decay, policy_scale, &policy_logger);*/
  /*cvc::sarsa::AnnealingSoftmaxPolicy learning_policy(policy_initial_temperature,
                                         &policy_logger);*/
  int num_non_learning_agents = agents.size();
  cvc::sarsa::MoneyScorer scorer;
  for (int i = 0; i < num_learning_agents; i++) {
    c.push_back(std::make_unique<Character>(i + num_non_learning_agents,
                                            money_dist(random_generator)));
    characters.push_back(c.back().get());
    characters.back()->traits_[kBackground] = background_dist(random_generator);
    characters.back()->traits_[kLanguage] = language_dist(random_generator);

    a.push_back(
        std::make_unique<cvc::sarsa::SARSAAgent<cvc::sarsa::MoneyScorer>>(
            &scorer, characters.back(), sarsa_action_factories,
            sarsa_response_factories, &learning_policy, n_steps));
    agents.push_back(a.back().get());
  }

  //set up game environment
  FILE* action_log = fopen("/tmp/action_log", "a");
  setvbuf(action_log, NULL, _IOLBF, 1024*10);
  Logger action_logger = Logger("action", action_log, WARN);
  action_logger.SetLogLevel(WARN);
  logger.Log(INFO, "creating CVC\n");
  CVC cvc(characters, &logger, random_generator);

  std::unique_ptr<DecisionEngine> d =
      DecisionEngine::Create(agents, &cvc, &action_logger);

  //run the simulation

  logger.Log(INFO, "running the game loop\n");
  auto start_tick = std::chrono::high_resolution_clock::now();

  cvc.LogState();
  int num_ticks = 10000;
  for (; cvc.Now() < num_ticks;) {
    d->RunOneGameLoop();

    if(cvc.Now() % 10000 == 0) {
      cvc.LogState();
    }
  }
  cvc.LogState();

  auto end_tick = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> loop_duration = end_tick - start_tick;

  logger.Log(INFO, "ran in %f seconds (%f ticks/sec)\n", loop_duration.count(), ((double)num_ticks)/loop_duration.count());

  //scf.WriteWeights();

  fclose(action_log);
  fclose(learn_log);
  fclose(policy_log);

  return 0;
}
