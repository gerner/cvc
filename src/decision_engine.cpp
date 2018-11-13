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

DecisionEngine::DecisionEngine(std::vector<ActionFactory*> action_factories,
                               CVC* cvc, FILE* action_log)
    : action_factories_(action_factories), cvc_(cvc), action_log_(action_log) {}

void DecisionEngine::GameLoop() {
  // TODO: don't just loop for some random hardcoded number of iterations
  for (; cvc_->Now() < 10000; cvc_->Tick()) {
    // 0. expire relationships
    cvc_->ExpireRelationships();
    // 1. evaluate queued actions
    EvaluateQueuedActions();
    // 2. choose actions for characters
    ChooseActions();

  }
  // TODO: expose state somehow to keep track of how stuff is going
  //  maybe also use the record of the state as training
  cvc_->PrintState();
}

std::vector<std::unique_ptr<Action>> DecisionEngine::EnumerateActions(
    Character* character) {
  std::vector<std::unique_ptr<Action>> ret;

  double sum_score = 0.0;
  for(auto action_factory : action_factories_) {
    sum_score += action_factory->EnumerateActions(cvc_, character, &ret);
  }

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
  for (auto character : cvc_->GetCharacters()) {
    // enumerate and score actions
    std::vector<std::unique_ptr<Action>> actions =
        EnumerateActions(character);

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
