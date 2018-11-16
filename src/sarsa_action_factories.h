#ifndef SARSA_ACTION_FACTORIES_H_
#define SARSA_ACTION_FACTORIES_H_

#include <vector>
#include <unordered_map>
#include <random>

#include "action.h"
#include "action_factories.h"
#include "decision_engine.h"

class SARSAActionFactory : public ActionFactory {
 public:
  SARSAActionFactory(std::vector<double> weights);

  void Learn(const Action* action, const Action* next_action) override;

 protected:
  double Score(const std::vector<double>& features);

 private:
  double n; //learning rate
  double g; //discount factor
  std::vector<double> weights_;
};

class SARSAGiveActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAGiveActionFactory> Create(std::mt19937 random_generator);

  SARSAGiveActionFactory(std::vector<double> weights);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSAAskActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAAskActionFactory> Create(std::mt19937 random_generator);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSAWorkActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAWorkActionFactory> Create(std::mt19937 random_generator);

  SARSAWorkActionFactory(std::vector<double> weights);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSATrivialActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSATrivialActionFactory> Create(std::mt19937 random_generator);

  SARSATrivialActionFactory(std::vector<double> weights);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
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

#endif
