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
#include "crunchedin/crunchedin.h"
#include "crunchedin/crunchedin_action_factories.h"

class ActionsFactory {
 public:
  ActionsFactory() {}

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

  template <class AF, class B>
  std::unique_ptr<B> CreateFactoryPtr() {
    return std::make_unique<AF>(AF::CreateLearner(
        num_learners_++, n_, g_, b1_, b2_, random_generator_, learn_logger_));
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

class CVCSetup {
 public:
  CVCSetup()
      : random_generator_(rd_()),
        money_dist_(10.0, 25.0),
        background_dist_(0, 10),
        language_dist_(0, 5),
        cf_({{"WorkAction", &waf_},
             {"GiveAction", &gaf_},
             {"AskAction", &aaf_},
             {"TrivialAction", &taf_}}) {

    learn_log_ = fopen("/tmp/learn_log", "a");
    setvbuf(learn_log_, NULL, _IOLBF, 1024*10);
    learn_logger_ = Logger("learner", learn_log_, INFO);

    policy_log_ = fopen("/tmp/policy_log", "a");
    setvbuf(policy_log_, NULL, _IOLBF, 1024*10);
    policy_logger_ = Logger("policy", policy_log_, WARN);

    action_log_ = fopen("/tmp/action_log", "a");
    setvbuf(action_log_, NULL, _IOLBF, 1024*10);
    action_logger_ = Logger("action", action_log_, WARN);

    f_ = ActionsFactory(n_, g_, b1_, b2_, &random_generator_, &learn_logger_);

    sarsa_action_factories_.push_back(
        f_.CreateFactoryPtr<cvc::sarsa::SARSAGiveActionFactory,
                            cvc::sarsa::ActionFactory>());
    sarsa_action_factories_.push_back(
        f_.CreateFactoryPtr<cvc::sarsa::SARSAAskActionFactory,
                            cvc::sarsa::ActionFactory>());
    sarsa_action_factories_.push_back(
        f_.CreateFactoryPtr<cvc::sarsa::SARSATrivialActionFactory,
                            cvc::sarsa::ActionFactory>());
    sarsa_action_factories_.push_back(
        f_.CreateFactoryPtr<cvc::sarsa::SARSAWorkActionFactory,
                            cvc::sarsa::ActionFactory>());

    sarsa_response_factories_.push_back(
        f_.CreateFactoryPtr<cvc::sarsa::SARSAAskSuccessResponseFactory,
                            cvc::sarsa::ResponseFactory>());
    sarsa_response_factories_.push_back(
        f_.CreateFactoryPtr<cvc::sarsa::SARSAAskFailureResponseFactory,
                            cvc::sarsa::ResponseFactory>());

    sarsa_response_map_ =
        std::unordered_map<std::string, std::set<cvc::sarsa::ResponseFactory*>>(
            {{"AskAction",
              {sarsa_response_factories_[0].get(),
               sarsa_response_factories_[1].get()}}});

    learning_policy_ = cvc::sarsa::DecayingEpsilonGreedyPolicy(
      policy_greedy_initial_e_, policy_greedy_scale_, &policy_logger_);
  }

  ~CVCSetup() {
    fclose(action_log_);
    fclose(learn_log_);
    fclose(policy_log_);
  }

  void AddHeuristicAgents(size_t num_heuristic_agents) {

    size_t num_characters = c_.size();
    for (size_t i = 0; i < num_heuristic_agents; i++) {
      c_.push_back(std::make_unique<Character>(num_characters + i,
                                              money_dist_(random_generator_)));
      a_.push_back(
          std::make_unique<HeuristicAgent>(c_.back().get(), &cf_, &rf_, &pdp_));
    }
  }

  void AddLearningAgents(size_t num_learning_agents) {

    std::vector<cvc::sarsa::ActionFactory*> action_factories;
    for(auto& factory : sarsa_action_factories_) {
      action_factories.push_back(factory.get());
    }

    size_t num_characters = c_.size();
    for (size_t i = 0; i < num_learning_agents; i++) {
      c_.push_back(std::make_unique<Character>(i + num_characters,
                                              money_dist_(random_generator_)));
      a_.push_back(
          std::make_unique<cvc::sarsa::SARSAAgent<cvc::sarsa::MoneyScorer>>(
              &scorer_, c_.back().get(), action_factories,
              sarsa_response_map_, &learning_policy_, n_steps_));
    }
  }

  void SetUpEnvironment() {
    std::vector<Character*> characters;
    for(auto& character : c_) {
      characters.push_back(character.get());
    }

    std::vector<Agent*> agents;
    for(auto& agent : a_) {
      agents.push_back(agent.get());
    }

    cvc_ = CVC(characters, &logger_, random_generator_);
    d_ = DecisionEngine(agents, &cvc_, &action_logger_);
  }

  CVC* GetCVC() {
    return &cvc_;
  }

  DecisionEngine* GetDecisionEngine() {
    return &d_;
  }

 private:

  std::random_device rd_;
  std::mt19937 random_generator_;

  std::uniform_real_distribution<> money_dist_;
  std::uniform_int_distribution<> background_dist_;
  std::uniform_int_distribution<> language_dist_;

  std::vector<std::unique_ptr<Character>> c_;
  std::vector<std::unique_ptr<Agent>> a_;

  CVC cvc_;
  DecisionEngine d_;

  Logger logger_;
  FILE* action_log_;
  Logger action_logger_;
  FILE* learn_log_;
  Logger learn_logger_;
  FILE* policy_log_;
  Logger policy_logger_;

  //heuristic agent state
  GiveActionFactory gaf_;
  AskActionFactory aaf_;
  WorkActionFactory waf_;
  TrivialActionFactory taf_;
  CompositeActionFactory cf_;
  AskResponseFactory rf_;
  ProbDistPolicy pdp_;

  //learning agent state
  //double policy_greedy_e = 0.05;
  double policy_greedy_initial_e_ = 0.5;
  double policy_greedy_scale_ = 0.1;
  //double policy_temperature_ = 0.2;
  //double policy_initial_temperature_ = 50.0;
  //double policy_decay_ = 0.001;
  //double policy_scale_ = 0.001;
  double n_ = 0.001;
  double b1_ = 0.9;
  double b2_= 0.999;
  double g_= 0.9;
  int n_steps_ = 100;

  ActionsFactory f_;

  std::vector<std::unique_ptr<cvc::sarsa::ActionFactory>>
      sarsa_action_factories_;
  std::vector<std::unique_ptr<cvc::sarsa::ResponseFactory>>
      sarsa_response_factories_;
  std::unordered_map<std::string, std::set<cvc::sarsa::ResponseFactory*>>
      sarsa_response_map_;

  cvc::sarsa::DecayingEpsilonGreedyPolicy learning_policy_;

  cvc::sarsa::MoneyScorer scorer_;
};

int main(int argc, char** argv) {
  Logger logger;
  logger.Log(INFO, "setting up Characters\n");

  int num_heuristic_agents = 0;
  int num_learning_agents = 25;
  CVCSetup setup;
  setup.AddHeuristicAgents(num_heuristic_agents);
  setup.AddLearningAgents(num_learning_agents);
  setup.SetUpEnvironment();

  CVC* cvc = setup.GetCVC();
  DecisionEngine* d = setup.GetDecisionEngine();

  //run the simulation

  logger.Log(INFO, "running the game loop\n");
  auto start_tick = std::chrono::high_resolution_clock::now();

  cvc->LogState();
  int num_ticks = 10000;
  for (; cvc->Now() < num_ticks;) {
    d->RunOneGameLoop();

    if(cvc->Now() % 10000 == 0) {
      cvc->LogState();
    }
  }
  cvc->LogState();

  auto end_tick = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> loop_duration = end_tick - start_tick;

  logger.Log(INFO, "ran in %f seconds (%f ticks/sec)\n", loop_duration.count(), ((double)num_ticks)/loop_duration.count());

  return 0;
}
