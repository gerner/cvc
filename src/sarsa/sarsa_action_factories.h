#ifndef SARSA_ACTION_FACTORIES_H_
#define SARSA_ACTION_FACTORIES_H_

#include <vector>
#include <unordered_map>
#include <random>
#include <memory>
#include <set>
#include <cassert>

#include "../decision_engine.h"
#include "sarsa_agent.h"
#include "sarsa_learner.h"

namespace cvc::sarsa {

double* StandardFeatures(CVC* cvc, Character* character, double* features);

double* TargetFeatures(CVC* cvc, Character* character, Character* target,
                       double* features);

class SARSAGiveActionFactory : public SARSAActionFactory<10> {
 public:
  SARSAGiveActionFactory(SARSALearner<10> learner)
      : SARSAActionFactory<10>(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override {

    if (character->GetMoney() > 10.0) {
      //choose a single target to potentially give to
      double sum_score = 0.0;
      for (Character* target : cvc->GetCharacters()) {
        if(target == character) {
          continue;
        }
        double features[10];

        //add this as an action choice
        actions->push_back(
            learner_.WrapAction(TargetFeatures(cvc, character, target, features),
                                 std::make_unique<GiveAction>(
                                     character, 0.0, target, 10.0)));
        sum_score += actions->back()->action_->GetScore();

      }
      return sum_score;
    }
    return 0.0;
  }
};

class SARSAAskActionFactory : public SARSAActionFactory<10> {
 public:
  SARSAAskActionFactory(SARSALearner<10> learner)
      : SARSAActionFactory<10>(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) {
    double sum_score = 0.0;
    for (Character* target : cvc->GetCharacters()) {
      // skip self
      if (character == target) {
        continue;
      }

      if (target->GetMoney() <= 10.0) {
        continue;
      }
      double features[10];
      //add this as an option to ask
      actions->push_back(learner_.WrapAction(
          TargetFeatures(cvc, character, target, features),
          std::make_unique<AskAction>(character, 0.0, target, 10.0)));
      sum_score += actions->back()->action_->GetScore();
    }
    return sum_score;
  }
};

class SARSAAskSuccessResponseFactory : public SARSAResponseFactory<10> {
 public:
  SARSAAskSuccessResponseFactory(SARSALearner<10> learner)
      : SARSAResponseFactory<10>(learner) {}

  double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) override {
    //action->GetTarget() is asking us for action->GetRequestAmount() money
    AskAction* ask_action = (AskAction*)action;

    if(character->GetMoney() < ask_action->GetRequestAmount()) {
      return 0.0;
    }

    double features[10];
    actions->push_back(learner_.WrapAction(
        TargetFeatures(cvc, character, ask_action->GetTarget(), features),
        std::make_unique<AskSuccessAction>(character, 0.0, ask_action->GetActor(),
                                           ask_action)));
    return actions->back()->action_->GetScore();
  }
};

class SARSAAskFailureResponseFactory : public SARSAResponseFactory<10> {
 public:
  SARSAAskFailureResponseFactory(SARSALearner<10> learner)
      : SARSAResponseFactory(learner) {}

  double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) override {
    //action->GetTarget() is asking us for action->GetRequestAmount() money
    AskAction* ask_action = (AskAction*)action;

    double features[10];
    actions->push_back(learner_.WrapAction(
        TargetFeatures(cvc, character, ask_action->GetTarget(), features),
        std::make_unique<TrivialResponse>(character, 0.0)));
    return actions->back()->action_->GetScore();
  }
};

class SARSAWorkActionFactory : public SARSAActionFactory<6> {
 public:
  SARSAWorkActionFactory(SARSALearner<6> learner)
      : SARSAActionFactory<6>(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override {
    double features[10];
    actions->push_back(
        learner_.WrapAction(StandardFeatures(cvc, character, features),
                             std::make_unique<WorkAction>(character, 0.0)));
    return actions->back()->action_->GetScore();
  }
};

class SARSATrivialActionFactory : public SARSAActionFactory<6> {
 public:
  SARSATrivialActionFactory(SARSALearner<6> learner)
      : SARSAActionFactory<6>(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override {
    double features[6];
    StandardFeatures(cvc, character, features);
    actions->push_back(learner_.WrapAction(features,
        std::make_unique<TrivialAction>(character, 0.0)));
    return actions->back()->action_->GetScore();
  }
};

class EpsilonGreedyPolicy : public SARSAActionPolicy {
 public:
  EpsilonGreedyPolicy(double epsilon, Logger* logger)
      : epsilon_(epsilon), logger_(logger){};

  std::unique_ptr<Experience> ChooseAction(
      std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
      Character* character) override;

 protected:
  double epsilon_;
  Logger* logger_;
};

class DecayingEpsilonGreedyPolicy : public EpsilonGreedyPolicy {
 public:
  DecayingEpsilonGreedyPolicy(double initial_epsilon, double scale,
                              Logger* logger)
      : EpsilonGreedyPolicy(initial_epsilon, logger),
        initial_epsilon_(initial_epsilon),
        scale_(scale){};
  std::unique_ptr<Experience> ChooseAction(
    std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
    Character* character) override {
    epsilon_ = initial_epsilon_ / sqrt((scale_ * cvc->Now()) + 1);
    return EpsilonGreedyPolicy::ChooseAction(actions, cvc, character);
  }

 private:
  double initial_epsilon_;
  double scale_;
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

} //namespace cvc::sarsa

#endif
