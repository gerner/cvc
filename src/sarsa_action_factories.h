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
      SARSALearner<10>* learner) {
    return std::make_unique<SARSAGiveActionFactory>(learner);
  }

  SARSAGiveActionFactory(SARSALearner<10>* learner)
      : learner_(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override;

 private:
  SARSALearner<10>* learner_;
};

class SARSAAskActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAAskActionFactory> Create(
      SARSALearner<10>* learner) {
    return std::make_unique<SARSAAskActionFactory>(learner);
  }

  SARSAAskActionFactory(SARSALearner<10>* learner)
      : learner_(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override;

 private:
  SARSALearner<10>* learner_;
};

class SARSAAskSuccessResponseFactory : public SARSAResponseFactory {
 public:
  static std::unique_ptr<SARSAAskSuccessResponseFactory> Create(
      SARSALearner<10>* learner) {
    return std::make_unique<SARSAAskSuccessResponseFactory>(learner);
  }


  SARSAAskSuccessResponseFactory(SARSALearner<10>* learner)
      : learner_(learner) {}

  double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) override;

 private:
  SARSALearner<10>* learner_;
};

class SARSAAskFailureResponseFactory : public SARSAResponseFactory {
 public:
  static std::unique_ptr<SARSAAskFailureResponseFactory> Create(
      SARSALearner<10>* learner) {
    return std::make_unique<SARSAAskFailureResponseFactory>(
        learner);
  }


  SARSAAskFailureResponseFactory(SARSALearner<10>* learner)
      : learner_(learner) {}

  double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) override;

 private:
  SARSALearner<10>* learner_;
};

class SARSAWorkActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAWorkActionFactory> Create(
      SARSALearner<6>* learner) {
    return std::make_unique<SARSAWorkActionFactory>(learner);
  }


  SARSAWorkActionFactory(SARSALearner<6>* learner)
      : learner_(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override;

 private:
  SARSALearner<6>* learner_;
};

class SARSATrivialActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSATrivialActionFactory> Create(
      SARSALearner<6>* learner) {
    return std::make_unique<SARSATrivialActionFactory>(learner);
  }


  SARSATrivialActionFactory(SARSALearner<6>* learner)
      : learner_(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override;

 private:
  SARSALearner<6>* learner_;
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

 protected:
  double temperature_;
  Logger* logger_;
};

class AnnealingSoftmaxPolicy : public SoftmaxPolicy {
 public:
  AnnealingSoftmaxPolicy(double initial_temperature, Logger* logger)
      : SoftmaxPolicy(initial_temperature, logger),
        initial_temperature_(initial_temperature){};

  std::unique_ptr<Experience> ChooseAction(
      std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
      Character* character) override;

 private:
  double initial_temperature_;
};

class GradSensitiveSoftmaxPolicy : public SoftmaxPolicy {
 public:
  GradSensitiveSoftmaxPolicy(double initial_temperature, double decay,
                             double scale, Logger* logger)
      : SoftmaxPolicy(initial_temperature, logger),
        decay_(decay),
        scale_(scale){};

  void UpdateGrad(double dL_dy, double y);

 private:
  double decay_;
  double scale_;
  double min_temperature_ = 0.05;
  double max_temperature_ = 10.0;
};

#endif
