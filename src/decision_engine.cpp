#include <stdio.h>
#include <vector>
#include <memory>
#include <random>
#include <cassert>
#include <sstream>
#include <iterator>
#include <algorithm>

#include "core.h"
#include "decision_engine.h"
#include "action.h"

ActionFactory::~ActionFactory() {}

bool ActionFactory::Respond(CVC* cvc, const Action* action) {
  //convenience base implementation for actions with no response mechanism

  // default should never be called (just there for actions with no targets)
  // and if there's no target, we can't find the corresponding agent for the
  // action to formulate a response
  // and any action factory that creates actions that require response should
  // implement its own Respond method.
  assert(nullptr == action->GetTarget());
  abort();
}

void ActionFactory::Learn(CVC* cvc, std::unique_ptr<Experience> experience) {
  //no default learning
}

std::unique_ptr<DecisionEngine> DecisionEngine::Create(
    std::vector<Agent*> agents, CVC* cvc, FILE* action_log) {
  std::unique_ptr<DecisionEngine> d =
      std::make_unique<DecisionEngine>(agents, cvc, action_log);

  return d;
}

DecisionEngine::DecisionEngine(std::vector<Agent*> agents,
                               CVC* cvc, FILE* action_log)
    : agents_(agents), cvc_(cvc), action_log_(action_log) {
  for (Agent* agent : agents_) {
    agent_lookup_[agent->character_] = agent;
  }
}

void DecisionEngine::RunOneGameLoop() {
  // 1. evaluate queued actions, s, a => r, s'
  EvaluateQueuedActions();

  // 2. tick the game forward
  cvc_->Tick();

  // 3. choose actions for characters, a'
  ChooseActions();

  // 4. learn from experiences
  Learn();
}

void DecisionEngine::EvaluateQueuedActions() {
  //experiences must be empty before we begin to evaluate actions
  assert(0 == experiences_.size());

  // go through list of all the queued actions
  for (auto& experience : queued_actions_) {

    Action* action = experience->action_.get();

    // ensure the action is still valid in the current state
    if (!action->IsValid(cvc_)) {
      // if it's not valid any more, just skip it
      cvc_->invalid_actions_++;
      LogInvalidAction(action);
      continue;
    }

    // TODO: handle actions that require a response
    // if a proposal, determine if the target accepts
    /*bool accept = true;
    if (action->GetTarget()) {
      accept = agent_lookup_[action->GetTarget()]->action_factory_->Respond(
          cvc_, action.get());
    }*/

    // spit out the action vector:
    LogAction(action);

    // let the action's effect play out
    //  this includes any character interaction
    action->TakeEffect(cvc_);

    // stick this (still partial) experience in the list of experiences for this
    // tick
    experiences_.push_back(std::move(experience));

    // we keep experiences for this turn in a heap cause we want these sorted
    std::push_heap(experiences_.begin(), experiences_.end(),
                   ExperienceByAgent());
  }
  //actions evaluated, so dump the list
  queued_actions_.clear();

  //other folks expect experiences to be sorted by agent
  std::sort_heap(experiences_.begin(), experiences_.end(), ExperienceByAgent());

  if (action_log_) {
    fflush(action_log_);
  }
}

void DecisionEngine::ChooseActions() {
  std::uniform_real_distribution<> dist(0.0, 1.0);
  // go through list of all characters
  for (Agent* agent : agents_) {
    //enumerate the optoins
    std::vector<std::unique_ptr<Action>> actions;
    agent->action_factory_->EnumerateActions(cvc_, agent->character_, &actions);

    // choose one according to the policy and store it, along with this agent in
    // a partial Experience which we will fill out later
    queued_actions_.push_back(std::make_unique<Experience>(
        agent, agent->policy_->ChooseAction(&actions, cvc_, agent->character_),
        nullptr));

    // now that we have a new action, we can update all partial
    // experiences for this agent with this updated action
    auto p = std::equal_range(experiences_.begin(), experiences_.end(),
                              agent, ExperienceByAgent());
    for(auto i = p.first; i != p.second; i++) {
      i->get()->next_action_ = queued_actions_.back()->action_.get();
    }
  }
}

void DecisionEngine::Learn() {
  for (auto& experience : experiences_) {
    assert(experience->action_);
    assert(experience->next_action_);

    //let the agent take ownership of this experience
    experience->agent_->action_factory_->Learn(cvc_, std::move(experience));
  }

  //all of the experiences have been moved elsewhere, ditch the empty pointers
  experiences_.clear();
}

void DecisionEngine::LogInvalidAction(const Action* action) {
    std::vector<double> features = action->GetFeatureVector();
    std::ostringstream s;
    std::copy(features.begin(), features.end(),
              std::ostream_iterator<double>(s, "\t"));

    //TODO: move to logging framework
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
