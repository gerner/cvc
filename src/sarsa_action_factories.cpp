#include <vector>
#include <cassert>
#include <random>
#include <limits>
#include <cmath>
#include <cstring>

#include "core.h"
#include "action.h"
#include "sarsa_action_factories.h"

SARSAActionFactory::SARSAActionFactory(double n, double g,
                                       std::vector<double> weights,
                                       Logger* learn_logger)
    : n_(n), g_(g), weights_(weights), learn_logger_(learn_logger) {}

void SARSAActionFactory::WriteWeights(FILE* weights_file) {
  size_t num_weights = weights_.size();
  fwrite(&num_weights, sizeof(size_t), 1, weights_file);
  for(double w : weights_) {
    fwrite(&w, sizeof(double), 1, weights_file);
  }
}

void SARSAActionFactory::ReadWeights(FILE* weights_file) {
  size_t count;
  int ret = fread(&count, sizeof(size_t), 1, weights_file);
  assert(1 == ret);
  assert(count == weights_.size());
  weights_.clear();
  for(size_t i=0; i<count; i++) {
    double w;
    fread(&w, sizeof(double), 1, weights_file);
    weights_.push_back(w);
  }
}

void SARSAActionFactory::Learn(CVC* cvc, const Action* action,
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

  learn_logger_->Log(
      INFO, "%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", cvc->Now(), d,
      action->GetScore(), truth_estimate, action->GetReward(),
      action->GetFeatureVector()[0], action->GetFeatureVector()[1], weights_[0],
      weights_[1]);

  // assert(action->GetScore() == Score(action->GetFeatureVector()));
  // update the weights
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
    double n, double g, std::mt19937 random_generator, Logger* learn_logger) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSATrivialActionFactory>(n, g, weights,
                                                     learn_logger);
}

SARSATrivialActionFactory::SARSATrivialActionFactory(
    double n, double g, std::vector<double> weights, Logger* learn_logger)
    : SARSAActionFactory(n, g, weights, learn_logger) {}

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
  std::vector<double> features({1.0, log(character->GetMoney())});
  double score = Score(features);
  actions->push_back(
      std::make_unique<TrivialAction>(character, score, features));
  return score;
}

std::unique_ptr<SARSAGiveActionFactory> SARSAGiveActionFactory::Create(
    double n, double g, std::mt19937 random_generator, Logger* learn_logger) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAGiveActionFactory>(n, g, weights, learn_logger);
}

SARSAGiveActionFactory::SARSAGiveActionFactory(double n, double g,
                                               std::vector<double> weights,
                                               Logger* learn_logger)
    : SARSAActionFactory(n, g, weights, learn_logger) {}

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
          10.0));
      return best_score;
    }
  }
  return 0.0;
}

std::unique_ptr<SARSAAskActionFactory> SARSAAskActionFactory::Create(
    double n, double g, std::mt19937 random_generator, Logger* learn_logger) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAAskActionFactory>(n, g, weights, learn_logger);
}

SARSAAskActionFactory::SARSAAskActionFactory(double n, double g,
                                             std::vector<double> weights,
                                             Logger* learn_logger)
    : SARSAActionFactory(n, g, weights, learn_logger) {}

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
    double n, double g, std::mt19937 random_generator, Logger* learn_logger) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSAWorkActionFactory>(n, g, weights, learn_logger);
}

SARSAWorkActionFactory::SARSAWorkActionFactory(double n, double g,
                                               std::vector<double> weights,
                                               Logger* learn_logger)
    : SARSAActionFactory(n, g, weights, learn_logger) {}

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
  std::vector<double> features({1.0, log(character->GetMoney())});
  double score = Score(features);
  actions->push_back(
      std::make_unique<WorkAction>(character, score, features));
  return score;
}

SARSACompositeActionFactory::SARSACompositeActionFactory(
    std::unordered_map<std::string, SARSAActionFactory*> factories,
    const char* weight_file)
    : factories_(factories), weight_file_(weight_file) {}

double SARSACompositeActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  double score = 0.0;
  for(const auto& factory : factories_) {
    score += factory.second->EnumerateActions(cvc, character, actions);
  }
  return score;
}

void SARSACompositeActionFactory::Learn(CVC* cvc, const Action* action,
                                   const Action* next_action) {
  //pass learning on to the appropriate child factory
  factories_[action->GetActionId()]->Learn(cvc, action, next_action);
}

void SARSACompositeActionFactory::ReadWeights() {
  FILE* weights = fopen(weight_file_, "r");

  if(!weights) {
    //if the file doesn't exist, just leave existing weights
    return;
  }

  size_t num_factories;
  int ret = fread(&num_factories, sizeof(size_t), 1, weights);
  assert(1 == ret);

  for(size_t i=0; i<num_factories; i++) {
    size_t name_length;
    ret = fread(&name_length, sizeof(size_t), 1, weights);
    assert(1 == ret);
    assert(1024-1 > name_length);
    char factory_name[1024];
    factory_name[name_length] = 0;
    ret = fread(factory_name, sizeof(char), name_length, weights);
    assert((size_t)ret == name_length);

    assert(factories_.find(factory_name) != factories_.end());
    factories_[factory_name]->ReadWeights(weights);
  }
  fclose(weights);
}

void SARSACompositeActionFactory::WriteWeights() {
  FILE* weights = fopen(weight_file_, "w");
  assert(weights);

  size_t num_factories = factories_.size();
  int ret = fwrite(&num_factories, sizeof(size_t), 1, weights);
  assert(ret);

  for(const auto& factory : factories_) {
    size_t name_length = strlen(factory.first.c_str());
    ret = fwrite(&name_length, sizeof(size_t), 1, weights);
    assert(ret > 0);
    ret = fwrite(factory.first.c_str(), sizeof(char), name_length, weights);
    assert(name_length == (size_t)ret);
    factory.second->WriteWeights(weights);
  }
  fclose(weights);
}

std::unique_ptr<Action> EpsilonGreedyPolicy::ChooseAction(
    std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
    Character* character) {
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

