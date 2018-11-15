#include <stdio.h>
#include <vector>
#include <memory>
#include <random>
#include <cassert>
#include <sstream>
#include <iterator>

#include "core.h"
#include "decision_engine.h"
#include "action.h"

ActionFactory::~ActionFactory() {}

DecisionEngine::DecisionEngine(std::vector<Agent*> agents,
                               CVC* cvc, FILE* action_log)
    : agents_(agents), cvc_(cvc), action_log_(action_log) {}

void DecisionEngine::GameLoop() {
  cvc_->LogState();
  // TODO: don't just loop for some random hardcoded number of iterations
  for (; cvc_->Now() < 10000; cvc_->Tick()) {
    // 0. expire relationships
    cvc_->ExpireRelationships();
    // 1. evaluate queued actions, s, a => r, s'
    EvaluateQueuedActions();
    // 2. choose actions for characters, a'
    ChooseActions();

    //TODO: on-line model training
    //we just walked through a training example for each action that go taken,
    //so we can apply that

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

    //we have one experience for each character

    /*for(Experience *experience : experiences_) {
      ActionFactory* action_factory = experience->agent->action_factory_;
      Action* action = experience->action;
      Action* next_action = experience->next_action;

      //compute (estimate) the error
      double d = action->GetReward() + g * next_action->GetScore() - action->GetScore();

      //update the weights
      for(int i = 0; i < action->GetFeatures().size(); i++) {
        action_factory->GetWeights()[i] = action_factory->GetWeights()[i] + n * d * action->GetFeatures()[i];
      }
    }*/
  }
  // TODO: expose state somehow to keep track of how stuff is going
  //  maybe also use the record of the state as training
  cvc_->LogState();
}

std::vector<std::unique_ptr<Action>> DecisionEngine::EnumerateActions(
    Agent* agent) {
  std::vector<std::unique_ptr<Action>> ret;

  double sum_score =
      agent->action_factory_->EnumerateActions(cvc_, agent->character_, &ret);

  //normalize scores
  double recomputed_sum_score = 0.0;
  for (auto& action : ret) {
    double score = action->GetScore();
    recomputed_sum_score += score;
    action->SetScore(score / sum_score);
  }

  assert(recomputed_sum_score == sum_score);

  return ret;
}

void DecisionEngine::ChooseActions() {
  std::uniform_real_distribution<> dist(0.0, 1.0);
  // go through list of all characters
  for (Agent* agent : agents_) {
    // enumerate and score actions
    std::vector<std::unique_ptr<Action>> actions =
        EnumerateActions(agent);

    //there must be at least one action to choose from (even if it's trivial)
    assert(!actions.empty());

    // choose one
    double choice = dist(*cvc_->GetRandomGenerator());
    double sum_score = 0;
    bool chose = false;

    // DecisionEngine is responsible for maintaining the lifecycle of the action
    // so we'll move it to our state and ditch the rest when they go out of
    // scope
    for (auto& action : actions) {
      sum_score += action->GetScore();
      if (choice < sum_score) {
        assert(action->IsValid(cvc_));
        // keep this one action
        this->queued_actions_.push_back(std::move(action));
        // rest of the actions will go out of scope and
        chose = true;
        break;
      }
    }

    //make sure that the action choices were a well formed distribution:
    //  sum of probs should not be more than 1 and we should make a choice
    assert(sum_score <= 1.0);
    assert(chose);
  }
}

void DecisionEngine::EvaluateQueuedActions() {
  // go through list of all the queued actions
  for (auto& action : queued_actions_) {
    // ensure the action is still valid in the current state
    if (!action->IsValid(cvc_)) {
      // if it's not valid any more, just skip it
      cvc_->invalid_actions_++;
      LogInvalidAction(action.get());
      continue;
    }

    // spit out the action vector:
    LogAction(action.get());

    // let the action's effect play out
    //  this includes any character interaction
    action->TakeEffect(cvc_);
  }

  if (action_log_) {
    fflush(action_log_);
  }

  // lastly, clear all the actions
  queued_actions_.clear();
}

void DecisionEngine::LogInvalidAction(const Action* action) {
    std::vector<double> features = action->GetFeatureVector();
    std::ostringstream s;
    std::copy(features.begin(), features.end(),
              std::ostream_iterator<double>(s, "\t"));

    fprintf(action_log_, "%d\t%d\t%f\t%s\t%s\t%f\t%s\n", cvc_->Now(),
            action->GetActor()->GetId(), action->GetActor()->GetMoney(),
            "INVALID", action->GetActionId(), action->GetScore(), s.str().c_str());
}

void DecisionEngine::LogAction(const Action* action) {
  //  tick
  //  user id
  //  user score
  //  action id
  //  action score
  //  feature vector
  if (this->action_log_) {
    std::vector<double> features = action->GetFeatureVector();
    std::ostringstream s;
    std::copy(features.begin(), features.end(),
              std::ostream_iterator<double>(s, "\t"));

    fprintf(action_log_, "%d\t%d\t%f\t%s\t%f\t%s\n", cvc_->Now(),
            action->GetActor()->GetId(), action->GetActor()->GetMoney(),
            action->GetActionId(), action->GetScore(), s.str().c_str());
  }
}
