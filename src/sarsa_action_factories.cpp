#include <vector>
#include <cassert>
#include <random>
#include <limits>
#include <cmath>

#include "core.h"
#include "action.h"
#include "sarsa_action_factories.h"

SARSAActionFactory::SARSAActionFactory(double n, double g,
                                       std::vector<double> weights)
    : n_(n), g_(g), weights_(weights) {}

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
  double truth_estimate = action->GetReward() + g_ * next_action->GetScore();
  double d = truth_estimate - action->GetScore();
  assert(!std::isinf(d));

  //printf("%f = %f + %f * %f - %f\n", d, action->GetReward(), g_, next_action->GetScore(), action->GetScore());

  //assert(action->GetScore() == Score(action->GetFeatureVector()));
  //update the weights
  assert(action->GetFeatureVector().size() == weights_.size());
  for(size_t i = 0; i < action->GetFeatureVector().size(); i++) {
    double weight_update = n_ * d * action->GetFeatureVector()[i];
    weights_[i] = weights_[i] + weight_update;
  }

  // check that we are doing it right:
  // error should get smaller
  //assert((truth_estimate - Score(action->GetFeatureVector())) <= abs(truth_estimate - action->GetScore()));
  // according to Hendrickson, the difference between the old score and the new
  // one should be the error computed above times the learning rate
  //assert(action->GetScore() - Score(action->GetFeatureVector()) == d * n_);
}

double SARSAActionFactory::Score(const std::vector<double>& features) {
  assert(features.size() == weights_.size());
  double score = 0.0;
  for(size_t i = 0; i < features.size(); i++) {
    score += weights_[i] * features[i];
  }
  assert(!std::isinf(score));
  return score;
}

std::unique_ptr<SARSATrivialActionFactory> SARSATrivialActionFactory::Create(
    double n, double g, std::mt19937 random_generator) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSATrivialActionFactory>(n, g, weights);
}

SARSATrivialActionFactory::SARSATrivialActionFactory(
    double n, double g, std::vector<double> weights)
    : SARSAActionFactory(n, g, weights) {}

double SARSATrivialActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  /*std::vector<double> features(
      {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
       cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
       character->GetMoney(), cvc->GetOpinionByStats(character->GetId()).mean_,
       cvc->GetOpinionByStats(character->GetId()).stdev_,
       cvc->GetOpinionOfStats(character->GetId()).mean_,
       cvc->GetOpinionOfStats(character->GetId()).stdev_});*/
  std::vector<double> features({1.0, 1.0});
  double score = Score(features);
  actions->push_back(
      std::make_unique<TrivialAction>(character, score, features));
  return score;
}

std::unique_ptr<SARSAGiveActionFactory> SARSAGiveActionFactory::Create(
    double n, double g, std::mt19937 random_generator) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAGiveActionFactory>(n, g, weights);
}

SARSAGiveActionFactory::SARSAGiveActionFactory(double n, double g,
                                               std::vector<double> weights)
    : SARSAActionFactory(n, g, weights) {}

double SARSAGiveActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  if (character->GetMoney() > 10.0) {
    //choose a single target to potentially give to
    double best_score = std::numeric_limits<double>::lowest();
    Character* best_target = NULL;
    std::vector<double> best_features;
    for (Character* target : cvc->GetCharacters()) {
      if(target == character) {
        continue;
      }
      //TODO: consider some other features
      /*std::vector<double> features(
          {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
           cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
           character->GetMoney(),
           cvc->GetOpinionByStats(character->GetId()).mean_,
           cvc->GetOpinionByStats(character->GetId()).stdev_,
           cvc->GetOpinionOfStats(character->GetId()).mean_,
           cvc->GetOpinionOfStats(character->GetId()).stdev_,
           character->GetOpinionOf(target), target->GetOpinionOf(character),
           target->GetMoney()});*/
      std::vector<double> features({1.0, target->GetOpinionOf(character)});
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
    double n, double g, std::mt19937 random_generator) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAAskActionFactory>(n, g, weights);
}

SARSAAskActionFactory::SARSAAskActionFactory(double n, double g,
                                             std::vector<double> weights)
    : SARSAActionFactory(n, g, weights) {}

double SARSAAskActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {

  Character* best_target = NULL;
  double best_score = std::numeric_limits<double>::lowest();
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
    /*std::vector<double> features(
        {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
         cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
         character->GetMoney(),
         cvc->GetOpinionByStats(character->GetId()).mean_,
         cvc->GetOpinionByStats(character->GetId()).stdev_,
         cvc->GetOpinionOfStats(character->GetId()).mean_,
         cvc->GetOpinionOfStats(character->GetId()).stdev_,
         character->GetOpinionOf(target), target->GetOpinionOf(character),
         target->GetMoney()});*/
    std::vector<double> features({1.0, target->GetOpinionOf(character)});
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
    double n, double g, std::mt19937 random_generator) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAWorkActionFactory>(n, g, weights);
}

SARSAWorkActionFactory::SARSAWorkActionFactory(double n, double g,
                                               std::vector<double> weights)
    : SARSAActionFactory(n, g, weights) {}

double SARSAWorkActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  /*std::vector<double> features(
      {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
       cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
       character->GetMoney(), cvc->GetOpinionByStats(character->GetId()).mean_,
       cvc->GetOpinionByStats(character->GetId()).stdev_,
       cvc->GetOpinionOfStats(character->GetId()).mean_,
       cvc->GetOpinionOfStats(character->GetId()).stdev_});*/
  std::vector<double> features({1.0, 1.0});
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
    double best_score = std::numeric_limits<double>::lowest();
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

