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
  // move the game forward by one tick
  // we'll let agents play out actions, the game state will update agents will
  // choose new actions.
  // during this process agents will accumulate a set of experiences which they
  // then have the opportunity to learn from

  // 1. evaluate queued actions, s, a => r, s'
  // note, during this phase we might have some interactions between characters,
  // facilitated by their agents.
  EvaluateQueuedActions();

  // 2. tick the game forward
  cvc_->Tick();

  // 3. choose independent actions for characters, a'
  ChooseActions();

  // 4. learn from experiences accumulated during this tick
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

    // handle actions that require a response
    // if a proposal, choose how the target responds
    if (action->RequiresResponse()) {
      assert(action->GetTarget());
      assert(agent_lookup_.find(action->GetTarget()) != agent_lookup_.end());
      Agent* responding_agent = agent_lookup_[action->GetTarget()];

      std::vector<std::unique_ptr<Action>> responses;

      //TODO: agents should always be able to explicitly respond
      queued_actions_.push_back(std::make_unique<Experience>(
          responding_agent, responding_agent->Respond(cvc_, action), nullptr));
    }

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
  // go through list of all characters
  for (Agent* agent : agents_) {
    queued_actions_.push_back(std::make_unique<Experience>(agent, agent->ChooseAction(cvc_), nullptr));

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
    experience->agent_->Learn(cvc_, std::move(experience));
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
