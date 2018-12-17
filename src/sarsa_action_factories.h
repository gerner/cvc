#ifndef SARSA_ACTION_FACTORIES_H_
#define SARSA_ACTION_FACTORIES_H_

#include <vector>
#include <unordered_map>
#include <random>

#include "action.h"
#include "action_factories.h"
#include "decision_engine.h"

class SARSALearner {
 public:
  SARSALearner(double n, double g, std::vector<double> weights,
               Logger* learn_logger);
  void Learn(CVC* cvc, std::unique_ptr<Experience> experience);

  void WriteWeights(FILE* weights_file);
  void ReadWeights(FILE* weights_file);

  double Score(const std::vector<double>& features);

 private:
  double n_; //learning rate
  double g_; //discount factor
  std::vector<double> weights_;

  Logger* learn_logger_;
};

class SARSAActionFactory {
 public:
  SARSAActionFactory(double n, double g, std::vector<double> weights,
                     Logger* learn_logger);

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) = 0;

  void Learn(CVC* cvc, std::unique_ptr<Experience> experience) {
    learner_.Learn(cvc, std::move(experience));
  }

  double Score(const std::vector<double>& features) {
    return learner_.Score(features);
  }

  void WriteWeights(FILE* weights_file) {
    learner_.WriteWeights(weights_file);
  }
  void ReadWeights(FILE* weights_file) {
    learner_.ReadWeights(weights_file);
  }

 private:
  SARSALearner learner_;
};

class SARSAResponseFactory {
 public:
  virtual double Respond(CVC* cvc, Action* action,
                         std::vector<std::unique_ptr<Action>>* actions) = 0;
};

class SARSAGiveActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAGiveActionFactory> Create(
      double n, double g, std::mt19937 random_generator, Logger* learn_logger);

  SARSAGiveActionFactory(double n, double g, std::vector<double> weights,
                         Logger* learn_logger);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSAAskActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAAskActionFactory> Create(
      double n, double g, std::mt19937 random_generator, Logger* learn_logger);

  SARSAAskActionFactory(double n, double g, std::vector<double> weights,
                        Logger* learn_logger);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSAWorkActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAWorkActionFactory> Create(
      double n, double g, std::mt19937 random_generator, Logger* learn_logger);

  SARSAWorkActionFactory(double n, double g, std::vector<double> weights,
                         Logger* learn_logger);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSATrivialActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSATrivialActionFactory> Create(
      double n, double g, std::mt19937 random_generator, Logger* learn_logger);

  SARSATrivialActionFactory(double n, double g, std::vector<double> weights,
                            Logger* learn_logger);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSACompositeActionFactory {
 public:
  SARSACompositeActionFactory(
      std::unordered_map<std::string, SARSAActionFactory*> factories,
      const char* weight_file);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions);

  void Learn(CVC* cvc, std::unique_ptr<Experience> experience);

  void ReadWeights();
  void WriteWeights();

 private:
  std::unordered_map<std::string, SARSAActionFactory*> factories_;
  const char* weight_file_;
};

class EpsilonGreedyPolicy : public ActionPolicy {
 public:
  EpsilonGreedyPolicy(double epsilon) : epsilon_(epsilon){};

  std::unique_ptr<Action> ChooseAction(
      std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
      Character* character) override;

 private:
  double epsilon_;
};

class SARSAAgent : public Agent {
 public:
  SARSAAgent(Character* character, SARSACompositeActionFactory* action_factory,
             SARSAResponseFactory* response_factory, ActionPolicy* policy)
      : Agent(character),
        action_factory_(action_factory),
        response_factory_(response_factory),
        policy_(policy) {}

  std::unique_ptr<Action> ChooseAction(CVC* cvc) override {
    // list the choices of actions
    std::vector<std::unique_ptr<Action>> actions;
    action_factory_->EnumerateActions(cvc, character_, &actions);

    // choose one according to the policy and store it, along with this agent in
    // a partial Experience which we will fill out later
    return policy_->ChooseAction(&actions, cvc, character_);
  }

  std::unique_ptr<Action> Respond(CVC* cvc, Action* action) override {
     //TODO: heuristic response TBD
     return nullptr;
  }

  // TODO: learn
  void Learn(CVC* cvc, std::unique_ptr<Experience> experience) override {
    action_factory_->Learn(cvc, std::move(experience));
  }


 private:
  SARSACompositeActionFactory* action_factory_;
  SARSAResponseFactory* response_factory_;
  ActionPolicy* policy_;
};

#endif
