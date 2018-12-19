#include <vector>
#include <cassert>
#include <random>
#include <limits>
#include <cmath>
#include <cstring>
#include <memory>

#include "core.h"
#include "action.h"
#include "sarsa_agent.h"

void SARSALearner::ReadWeights(
    const char* weight_file,
    std::unordered_map<std::string, SARSALearner*> learners) {
  FILE* weights = fopen(weight_file, "r");

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

    assert(learners.find(factory_name) != learners.end());
    learners[factory_name]->ReadWeights(weights);
  }
  fclose(weights);
}

void SARSALearner::WriteWeights(
    const char* weight_file,
    std::unordered_map<std::string, SARSALearner*> learners) {
  FILE* weights = fopen(weight_file, "w");
  assert(weights);

  size_t num_factories = learners.size();
  int ret = fwrite(&num_factories, sizeof(size_t), 1, weights);
  assert(ret);

  for(const auto& factory : learners) {
    size_t name_length = strlen(factory.first.c_str());
    ret = fwrite(&name_length, sizeof(size_t), 1, weights);
    assert(ret > 0);
    ret = fwrite(factory.first.c_str(), sizeof(char), name_length, weights);
    assert(name_length == (size_t)ret);
    factory.second->WriteWeights(weights);
  }
  fclose(weights);
}

void SARSALearner::WriteWeights(FILE* weights_file) {
  size_t num_weights = weights_.size();
  fwrite(&num_weights, sizeof(size_t), 1, weights_file);
  for(double w : weights_) {
    fwrite(&w, sizeof(double), 1, weights_file);
  }
}

void SARSALearner::ReadWeights(FILE* weights_file) {
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

std::unique_ptr<SARSALearner> SARSALearner::Create(double n, double g,
                                                   std::mt19937& random_generator,
                                                   size_t num_features,
                                                   Logger* learn_logger) {
  std::vector<double> weights;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < 2; i++) {
    weights.push_back(weight_dist(random_generator));
  }

  return std::make_unique<SARSALearner>(n, g, weights, learn_logger);
}

SARSALearner::SARSALearner(double n, double g, std::vector<double> weights,
                           Logger* learn_logger)
    : n_(n), g_(g), weights_(weights), learn_logger_(learn_logger) {}

void SARSALearner::Learn(CVC* cvc, Experience* experience) {
  Action* action = experience->action_.get();
  Action* next_action = experience->next_action_;

  assert(action);
  assert(next_action);

  //SARSA-FA:
  //Q(s, a) = r + g * Q(s', a')
  //where Q(s, a) was estimated
  //we had some error on that which we can gradient descend
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
  double truth_estimate = experience->reward_ + g_ * next_action->GetScore();
  double d = truth_estimate - action->GetScore();
  assert(!std::isinf(d));

  //log:
  // tick
  // action
  // reward
  // estimated score
  // r + g * next estimated score
  // d
  // |
  // features
  // |
  // weights
  learn_logger_->Log(
      INFO, "%d\t%s\t%f\t%f\t%f\t%f\t|\t%f\t%f\t|\t%f\t%f\n",
      cvc->Now(), action->GetActionId(), experience->reward_,
      action->GetScore(), truth_estimate, d,
      action->GetFeatureVector()[0], action->GetFeatureVector()[1],
      weights_[0], weights_[1]);

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

double SARSALearner::Score(const std::vector<double>& features) {
  //first feature had beter be bias term
  assert(features[0] == 1.0);

  assert(features.size() == weights_.size());
  double score = 0.0;
  for(size_t i = 0; i < features.size(); i++) {
    score += weights_[i] * features[i];
  }
  assert(!std::isinf(score));
  return score;
}

SARSAActionFactory::SARSAActionFactory(std::unique_ptr<SARSALearner> learner)
    : learner_(std::move(learner)) {}

Action* SARSAAgent::ChooseAction(CVC* cvc) {
  //if we have what was previously the next action, stick it in the set of
  //experiences
  if(next_action_) {
    experiences_.push_back(std::move(next_action_));
  }

  // list the choices of actions
  std::vector<std::unique_ptr<Experience>> actions;

  double score = 0.0;
  for (SARSAActionFactory* factory : action_factories_) {
    score += factory->EnumerateActions(cvc, character_, &actions);
  }

  // choose one according to the policy and store it, along with this agent in
  // a partial Experience which we will fill out later
  next_action_ = policy_->ChooseAction(&actions, cvc, character_);

  //TODO: should really support other kinds of objectives than just money
  next_action_->score_ = character_->GetMoney();

  return next_action_->action_.get();
}

Action* SARSAAgent::Respond(CVC* cvc, Action* action) {
  //1. find the appropriate response factory
  assert(response_factories_.find(action->GetActionId()) !=
         response_factories_.end());

  //2. ask it to enumerate some (scored) responses
  std::vector<std::unique_ptr<Experience>> actions;

  double score = 0.0;
  for (SARSAResponseFactory* factory :
       response_factories_[action->GetActionId()]) {
    score += factory->Respond(cvc, character_, action, &actions);
  }

  //3. choose
  experiences_.push_back(policy_->ChooseAction(&actions, cvc, character_));

  //TODO: should really support other kinds of objectives than just money
  experiences_.back()->score_ = character_->GetMoney();
  return experiences_.back()->action_.get();
}

void SARSAAgent::Learn(CVC* cvc) {
  // TODO: consider n-step SARSA, or modify things to conisder just the n-step
  // sequence of rewards (short-sighted learner)
  for(auto& experience : experiences_) {
    //TODO: shouldn't just use money
    experience->reward_ = character_->GetMoney() - experience->score_;
    experience->next_action_ = next_action_->action_.get();
    experience->learner_->Learn(cvc, experience.get());
  }

  experiences_.clear();
}

