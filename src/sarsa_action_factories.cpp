#include <vector>
#include <cassert>
#include <cfloat>
#include <random>

#include "core.h"
#include "action.h"
#include "sarsa_action_factories.h"

SARSAActionFactory::SARSAActionFactory(std::vector<double> weights)
    : weights_(weights) {}

void SARSAActionFactory::Learn(const Action* action,
                               const Action* next_action) {

  //SARSA-FA:
  //experience: (s, a, r, s', a')
  //  d = r + g*Q(s', a') - Q(s, a)
  //  for each w_i:
  //      w_i = w_i + n * d * F_i(s, a)
  //
  //where
  //r is the reward that was calculated during EvaluteQueuedActions
  //g is the discount factor (discounting future returns)
  //n is the learning rate (gradient descent step-size)
  //Q(s, a) is the score of a (previous estimate of future score)
  //Q(s', a') is the score of a' (current estimate of future score)
  //w are the weights that were used to choose a
  //F(s, a) are the features that go with a

  //compute (estimate) the error
  double d = action->GetReward() + g * next_action->GetScore() - action->GetScore();

  //update the weights
  assert(action->GetFeatureVector().size() == weights_.size());
  for(size_t i = 0; i < action->GetFeatureVector().size(); i++) {
    weights_[i] = weights_[i] + n * d * action->GetFeatureVector()[i];
  }
}

double SARSAActionFactory::Score(const std::vector<double>& features) {
  assert(features.size() == weights_.size());
  double score = 0.0;
  for(size_t i = 0; i < features.size(); i++) {
    score += weights_[i] * features[i];
  }
  return score;
}

std::unique_ptr<SARSATrivialActionFactory> SARSATrivialActionFactory::Create(
    std::mt19937 random_generator) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 10; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSATrivialActionFactory>(weights);
}

SARSATrivialActionFactory::SARSATrivialActionFactory(
    std::vector<double> weights)
    : SARSAActionFactory(weights) {}

double SARSATrivialActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  std::vector<double> features(
      {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
       cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
       character->GetMoney(), cvc->GetOpinionByStats(character->GetId()).mean_,
       cvc->GetOpinionByStats(character->GetId()).stdev_,
       cvc->GetOpinionOfStats(character->GetId()).mean_,
       cvc->GetOpinionOfStats(character->GetId()).stdev_});
  double score = Score(features);
  actions->push_back(
      std::make_unique<TrivialAction>(character, score, features));
  return score;
}

std::unique_ptr<SARSAGiveActionFactory> SARSAGiveActionFactory::Create(
    std::mt19937 random_generator) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 13; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAGiveActionFactory>(weights);
}

SARSAGiveActionFactory::SARSAGiveActionFactory(
    std::vector<double> weights)
    : SARSAActionFactory(weights) {}

double SARSAGiveActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  if (character->GetMoney() > 10.0) {
    //choose a single target to potentially give to
    double best_score = -DBL_MAX;
    Character* best_target = NULL;
    std::vector<double> best_features;
    for (Character* target : cvc->GetCharacters()) {
      if(target == character) {
        continue;
      }
      //TODO: consider some other features
      std::vector<double> features(
          {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
           cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
           character->GetMoney(),
           cvc->GetOpinionByStats(character->GetId()).mean_,
           cvc->GetOpinionByStats(character->GetId()).stdev_,
           cvc->GetOpinionOfStats(character->GetId()).mean_,
           cvc->GetOpinionOfStats(character->GetId()).stdev_,
           character->GetOpinionOf(target), target->GetOpinionOf(character),
           target->GetMoney()});
      double score = Score(features);
      if(score > best_score) {
        best_score = score;
        best_features = features;
        best_target = target;
      }
    }

    if (best_target) {
      actions->push_back(std::make_unique<GiveAction>(
          character, best_score, best_features, best_target,
          character->GetMoney() * .1));
      return best_score;
    }
  }
  return 0.0;
}

std::unique_ptr<SARSAAskActionFactory> SARSAAskActionFactory::Create(
    std::mt19937 random_generator) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 13; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAAskActionFactory>(weights);
}

SARSAAskActionFactory::SARSAAskActionFactory(
    std::vector<double> weights)
    : SARSAActionFactory(weights) {}

double SARSAAskActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {

  Character* best_target = NULL;
  double best_score = 0.0;
  std::vector<double> best_features;
  for (Character* target : cvc->GetCharacters()) {
    // skip self
    if (character == target) {
      continue;
    }

    if (target->GetMoney() <= 10.0) {
      continue;
    }
    //TODO: consider some other features
    std::vector<double> features(
        {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
         cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
         character->GetMoney(),
         cvc->GetOpinionByStats(character->GetId()).mean_,
         cvc->GetOpinionByStats(character->GetId()).stdev_,
         cvc->GetOpinionOfStats(character->GetId()).mean_,
         cvc->GetOpinionOfStats(character->GetId()).stdev_,
         character->GetOpinionOf(target), target->GetOpinionOf(character),
         target->GetMoney()});
    double score = Score(features);
    if(score > best_score) {
      best_score = score;
      best_features = features;
      best_target = target;
    }
  }
  if (best_target) {
    actions->push_back(std::make_unique<AskAction>(
        character, best_score, best_features, best_target,
        10.0));
    return best_score;
  }
  return 0.0;
}
std::unique_ptr<SARSAWorkActionFactory> SARSAWorkActionFactory::Create(
    std::mt19937 random_generator) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 10; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAWorkActionFactory>(weights);
}

SARSAWorkActionFactory::SARSAWorkActionFactory(
    std::vector<double> weights)
    : SARSAActionFactory(weights) {}

double SARSAWorkActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  std::vector<double> features(
      {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
       cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
       character->GetMoney(), cvc->GetOpinionByStats(character->GetId()).mean_,
       cvc->GetOpinionByStats(character->GetId()).stdev_,
       cvc->GetOpinionOfStats(character->GetId()).mean_,
       cvc->GetOpinionOfStats(character->GetId()).stdev_});
  double score = Score(features);
  actions->push_back(
      std::make_unique<WorkAction>(character, score, features));
  return score;
}

std::unique_ptr<Action> EpsilonGreedyPolicy::ChooseAction(std::vector<std::unique_ptr<Action>>* actions, CVC* cvc, Character* character) {
  // choose best action with prob 1-epsilon and a uniform random action with
  // prob epsilon

  assert(actions->size() > 0);

  //best or random?
  std::uniform_real_distribution<> dist(0.0, 1.0);
  if(dist(*cvc->GetRandomGenerator()) > epsilon_) {
    //best choice
    double best_score = -DBL_MAX;
    std::unique_ptr<Action>* best_action = nullptr;
    for(std::unique_ptr<Action>& action : *actions) {
      if(action->GetScore() > best_score) {
        best_score = action->GetScore();
        best_action = &action;
      }
    }
    assert(best_action);
    return std::move(*best_action);
  } else {
    //random choice
    return std::move((*actions)[dist(*cvc->GetRandomGenerator()) * actions->size()]);
  }
}

