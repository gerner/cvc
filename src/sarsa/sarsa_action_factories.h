#ifndef SARSA_ACTION_FACTORIES_H_
#define SARSA_ACTION_FACTORIES_H_

#include <vector>
#include <unordered_map>
#include <random>
#include <memory>
#include <set>
#include <cassert>
#include <array>
#include <limits>

#include "../decision_engine.h"
#include "sarsa_agent.h"
#include "sarsa_learner.h"

namespace cvc::sarsa {

template <size_t N>
std::array<double, N> StandardFeatures(CVC* cvc, Character* character,
                                       std::array<double, N> features) {
  features[0] = 1.0; //bias
  features[1] = 0.0;//character->GetMoney();//log(character->GetMoney());
  features[2] = 0.0;//log(cvc->GetMoneyStats().mean_);
  features[3] = 0.0;//cvc->GetOpinionStats().mean_/100.0;
  features[4] = 0.0;//cvc->GetOpinionByStats(character->GetId()).mean_/100.0;
  features[5] = 0.0;//cvc->GetOpinionOfStats(character->GetId()).mean_/100.0;

  return features;
}

template <size_t N>
std::array<double, N> TargetFeatures(CVC* cvc, Character* character,
                                     Character* target,
                                     std::array<double, N> features) {
  StandardFeatures(cvc, character, features);
  features[6] = 0.0;//character->GetOpinionOf(target) / 100.0;
  features[7] = 0.0;//target->GetOpinionOf(character) / 100.0;
  features[8] = 0.0;//target->GetMoney();//log(target->GetMoney());
  //TODO: this should be relationship between character and target money
  features[9] = 0.0;

  return features;
}

class SARSAGiveActionFactory : public SARSAActionFactory<10> {
 public:
  SARSAGiveActionFactory(SARSALearner<10> learner)
      : SARSAActionFactory<10>(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) override {

    double best_score = std::numeric_limits<double>::lowest();
    std::unique_ptr<Experience> best_action = nullptr;

    if (character->GetMoney() > 10.0) {
      //choose a single target to potentially give to
      for (Character* target : cvc->GetCharacters()) {
        if(target == character) {
          continue;
        }
        std::array<double, 10> features;

        std::unique_ptr<Experience> action = learner_.WrapAction(
            TargetFeatures(cvc, character, target, features),
            std::make_unique<GiveAction>(character, 0.0, target, 10.0));

        if(action->action_->GetScore() > best_score) {
          best_action = std::move(action);
          best_score = best_action->action_->GetScore();
        }
      }
      if(best_action) {
        actions->push_back(std::move(best_action));
        return actions->back()->action_->GetScore();
      } else {
        return 0.0;
      }
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
    double best_score = std::numeric_limits<double>::lowest();
    std::unique_ptr<Experience> best_action = nullptr;
    for (Character* target : cvc->GetCharacters()) {
      // skip self
      if (character == target) {
        continue;
      }

      if (target->GetMoney() <= 10.0) {
        continue;
      }
      std::array<double, 10> features;
      //add this as an option to ask
      std::unique_ptr<Experience> action = learner_.WrapAction(
          TargetFeatures(cvc, character, target, features),
          std::make_unique<AskAction>(character, 0.0, target, 10.0));
      if(action->action_->GetScore() > best_score) {
        best_action = std::move(action);
        best_score = best_action->action_->GetScore();
      }
    }

    if(best_action) {
      actions->push_back(std::move(best_action));
      return actions->back()->action_->GetScore();
    } else {
      return 0.0;
    }
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

    std::array<double, 10> features;
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

    std::array<double, 10> features;
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
    std::array<double, 6> features;
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
    std::array<double, 6> features;
    StandardFeatures(cvc, character, features);
    actions->push_back(learner_.WrapAction(features,
        std::make_unique<TrivialAction>(character, 0.0)));
    return actions->back()->action_->GetScore();
  }
};

class EpsilonGreedyPolicy : public SARSAActionPolicy {
 public:
  EpsilonGreedyPolicy() {}

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
  DecayingEpsilonGreedyPolicy() {}

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
