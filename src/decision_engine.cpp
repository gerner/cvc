#include <stdio.h>
#include <vector>
#include <memory>
#include <random>
#include <cassert>
#include <sstream>
#include <iterator>
#include <cfloat>

#include "core.h"
#include "decision_engine.h"
#include "action.h"

ActionFactory::~ActionFactory() {}

void ActionFactory::Learn(const Action* action, const Action* next_action) {
  //no default learning
}

std::unique_ptr<DecisionEngine> DecisionEngine::Create(
    std::vector<Agent*> agents, CVC* cvc, FILE* action_log) {
  std::unique_ptr<DecisionEngine> d =
      std::make_unique<DecisionEngine>(agents, cvc, action_log);

  //set up experiences
  for(Agent* agent : agents) {
    d->experiences_.push_back(std::make_unique<Experience>());
    d->experiences_.back()->agent_ = agent;
    d->experiences_.back()->action_ = nullptr;
    d->experiences_.back()->next_action_ = nullptr;
  }

  return d;
}

DecisionEngine::DecisionEngine(std::vector<Agent*> agents,
                               CVC* cvc, FILE* action_log)
    : agents_(agents), cvc_(cvc), action_log_(action_log) {}

void DecisionEngine::GameLoop() {
  cvc_->LogState();
  // TODO: don't just loop for some random hardcoded number of iterations

  // invariant: experiences_ represents the set of experiences including the
  // next action for all agents
  // and each experience represents the most recent experience (or null) of each
  // agent, including the next action the agent should take
  for (; cvc_->Now() < 100000; cvc_->Tick()) {
    assert(experiences_.size() == agents_.size());
    // 0. expire relationships
    cvc_->ExpireRelationships();
    // 1. evaluate queued actions, s, a => r, s'
    EvaluateQueuedActions();
    // 2. choose actions for characters, a'
    ChooseActions();

    // at this point we have a set of experiences that represent what we just
    // did (EvaluteQueuedActions) and what we expect to do next (ChooseActions)

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
    Learn();

    if(cvc_->Now() % 1000 == 0) {
      cvc_->LogState();
    }
  }
  cvc_->LogState();
}

void DecisionEngine::EvaluateQueuedActions() {
  // go through list of all the queued actions
  for (auto& experience : experiences_) {

    auto& action = experience->next_action_;

    if(action) {
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

    //we can forget about whatever the last action was at this point
    experience->action_ = std::move(experience->next_action_);
  }

  if (action_log_) {
    fflush(action_log_);
  }
}

void DecisionEngine::ChooseActions() {
  std::uniform_real_distribution<> dist(0.0, 1.0);
  // go through list of all characters
  for (auto& experience : experiences_) {
    //enumerate the optoins
    std::vector<std::unique_ptr<Action>> actions;
    experience->agent_->action_factory_->EnumerateActions(
        cvc_, experience->agent_->character_, &actions);

    //choose one according to the policy
    experience->next_action_ = experience->agent_->policy_->ChooseAction(
        &actions, cvc_, experience->agent_->character_);
  }
}

void DecisionEngine::Learn() {
  for(auto& experience : experiences_) {
    if (experience->action_) {
      experience->agent_->action_factory_->Learn(
          experience->action_.get(), experience->next_action_.get());
    }
  }
}

void DecisionEngine::LogInvalidAction(const Action* action) {
    std::vector<double> features = action->GetFeatureVector();
    std::ostringstream s;
    std::copy(features.begin(), features.end(),
              std::ostream_iterator<double>(s, "\t"));

    fprintf(action_log_, "%d\t%d\t%f\t%s\t%s\t%f\t%s\n", cvc_->Now(),
            action->GetActor()->GetId(), action->GetActor()->GetMoney(),
            "INVALID", action->GetActionId(), action->GetScore(),
            s.str().c_str());
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
