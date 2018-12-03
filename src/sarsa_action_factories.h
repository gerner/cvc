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
  SARSAActionFactory(double n, double g, std::vector<double> weights,
                     Logger* learn_logger);

  void Learn(CVC* cvc, const Action* action,
             const Action* next_action) override;

  void WriteWeights(FILE* weights_file);
  void ReadWeights(FILE* weights_file);

 protected:
  double Score(const std::vector<double>& features);

 private:
  double n_; //learning rate
  double g_; //discount factor
  std::vector<double> weights_;

  Logger* learn_logger_;
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

class SARSACompositeActionFactory : public ActionFactory {
 public:
  SARSACompositeActionFactory(
      std::unordered_map<std::string, SARSAActionFactory*> factories,
      const char* weight_file);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;

  void Learn(CVC* cvc, const Action* action,
             const Action* next_action) override;

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

#endif
