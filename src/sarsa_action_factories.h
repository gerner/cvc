#ifndef SARSA_ACTION_FACTORIES_H_
#define SARSA_ACTION_FACTORIES_H_

#include <vector>
#include <unordered_map>
#include <random>
#include <memory>
#include <set>
#include <cassert>

#include "decision_engine.h"
#include "sarsa_agent.h"

class SARSAGiveActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAGiveActionFactory> Create(
      std::unique_ptr<SARSALearner> learner);

  SARSAGiveActionFactory(std::unique_ptr<SARSALearner> learner);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override;
};

class SARSAAskActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAAskActionFactory> Create(
      std::unique_ptr<SARSALearner> learner);

  SARSAAskActionFactory(std::unique_ptr<SARSALearner> learner);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override;
};

class SARSAAskSuccessResponseFactory : public SARSAResponseFactory {
 public:
  static std::unique_ptr<SARSAAskSuccessResponseFactory> Create(
      std::unique_ptr<SARSALearner> learner);

  SARSAAskSuccessResponseFactory(std::unique_ptr<SARSALearner> learner);

  double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) override;
};

class SARSAAskFailureResponseFactory : public SARSAResponseFactory {
 public:
  static std::unique_ptr<SARSAAskFailureResponseFactory> Create(
      std::unique_ptr<SARSALearner> learner);

  SARSAAskFailureResponseFactory(std::unique_ptr<SARSALearner> learner);

  double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) override;
};

class SARSAWorkActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAWorkActionFactory> Create(
      std::unique_ptr<SARSALearner> learner);

  SARSAWorkActionFactory(std::unique_ptr<SARSALearner> learner);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override;
};

class SARSATrivialActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSATrivialActionFactory> Create(
      std::unique_ptr<SARSALearner> learner);

  SARSATrivialActionFactory(std::unique_ptr<SARSALearner> learner);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override;
};

class EpsilonGreedyPolicy : public SARSAActionPolicy {
 public:
  EpsilonGreedyPolicy(double epsilon, Logger* logger)
      : epsilon_(epsilon), logger_(logger){};

  std::unique_ptr<Experience> ChooseAction(
      std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
      Character* character) override;

 private:
  double epsilon_;
  Logger* logger_;
};

class SoftmaxPolicy : public SARSAActionPolicy {
 public:
  SoftmaxPolicy(double temperature, Logger* logger)
      : temperature_(temperature), logger_(logger){};

  std::unique_ptr<Experience> ChooseAction(
      std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
      Character* character) override;

 private:
  double temperature_;
  Logger* logger_;
};

#endif
