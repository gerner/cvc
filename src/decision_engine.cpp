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

DecisionEngine::DecisionEngine(CVC* cvc, FILE* action_log)
    : cvc_(cvc), action_log_(action_log) {}

void DecisionEngine::GameLoop() {
  // TODO: don't just loop for some random hardcoded number of iterations
  for (; cvc_->Now() < 1000; cvc_->Tick()) {
    // 0. expire relationships
    cvc_->ExpireRelationships();
    // 1. evaluate queued actions
    EvaluateQueuedActions();
    // 2. choose actions for characters
    ChooseActions();

    // TODO: expose state somehow to keep track of how stuff is going
    //  maybe also use the record of the state as training
    cvc_->PrintState();
  }
}

std::vector<std::unique_ptr<Action>> DecisionEngine::EnumerateActions(
    Character* character) {
  std::vector<std::unique_ptr<Action>> ret;

  // GiveAction
  Character* best_target = NULL;
  double worst_opinion = std::numeric_limits<double>::max();
  if (character->GetMoney() > 10.0) {
    for (Character* target : cvc_->GetCharacters()) {
      // skip self
      if (character == target) {
        continue;
      }

      // pick the character that likes us the least
      double opinion = target->GetOpinionOf(character);
      if (opinion < worst_opinion) {
        worst_opinion = opinion;
        best_target = target;
      }
    }
    if (best_target) {
      ret.push_back(std::make_unique<GiveAction>(
          character, 0.4, std::vector<double>({1.0, 0.2}), best_target,
          character->GetMoney() * 0.1));
    }
  }

  // AskAction
  best_target = NULL;
  double best_opinion = 0.0;
  for (Character* target : cvc_->GetCharacters()) {
    // skip self
    if (character == target) {
      continue;
    }

    if (target->GetMoney() <= 10.0) {
      continue;
    }

    // pick the character that likes us the least
    double opinion = target->GetOpinionOf(character);
    if (opinion > best_opinion) {
      best_opinion = opinion;
      best_target = target;
    }
  }
  if (best_target) {
    ret.push_back(std::make_unique<AskAction>(
        character, 0.4, std::vector<double>({0.7, 0.5}), best_target, 10.0));
  }

  ret.push_back(
      std::make_unique<TrivialAction>(character, 0.2, std::vector<double>()));

  return ret;
}

void DecisionEngine::ChooseActions() {
  std::uniform_real_distribution<> dist(0.0, 1.0);
  // go through list of all characters
  for (auto character : cvc_->GetCharacters()) {
    // run the decision making loop for each:
    double stop_prob = 0.7;
    while (true) {
      // check to see if we should stop choosing actions
      if (dist(*cvc_->GetRandomGenerator()) < stop_prob) {
        break;
      }

      // enumerate and score actions
      std::vector<std::unique_ptr<Action>> actions =
          EnumerateActions(character);

      // we assume if there's no actions there will not be any this tick
      if (actions.empty()) {
        break;
      }

      // choose one
      double choice = dist(*cvc_->GetRandomGenerator());
      double sum_score = 0;
      // CVC is responsible for maintaining the lifecycle of the action
      for (auto& action : actions) {
        sum_score += action->GetScore();
        if (choice < sum_score) {
          assert(action->IsValid(cvc_));
          // keep this one action
          this->queued_actions_.push_back(std::move(action));
          // rest of the actions will go out of scope and
          break;
        }
      }

      // update stop_prob
      stop_prob = 1.0 - (1.0 - stop_prob) * 0.2;
    }
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
            typeid(*action).name(), action->GetScore(), s.str().c_str());
  }
}
