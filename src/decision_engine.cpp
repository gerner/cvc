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
    agent_lookup_[agent->GetCharacter()] = agent;
  }
}

void DecisionEngine::RunOneGameLoop() {
  // for each agent:
  //    choose what to do
  //    play it out, including proposals, responses, etc.
  //    let the agent know the tick as progressed (and it can do whatever it
  //    wants)

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
  // go through list of all the queued actions
  for (auto& action : queued_actions_) {

    // ensure the action is still valid in the current state
    if (!action->IsValid(cvc_)) {
      // if it's not valid any more, just skip it
      cvc_->invalid_actions_++;
      LogInvalidAction(action);
    } else {

      // handle actions that require a response
      // if a proposal, choose how the target responds
      if (action->RequiresResponse()) {
        assert(action->GetTarget());
        assert(agent_lookup_.find(action->GetTarget()) != agent_lookup_.end());
        Agent* responding_agent = agent_lookup_[action->GetTarget()];

        std::vector<std::unique_ptr<Action>> responses;

        //TODO: agents should always be able to explicitly respond
        Action* response = responding_agent->Respond(cvc_, action);
        assert(response);
        queued_actions_.push_back(response);
      }

      // let the action's effect play out
      //  this includes any character interaction
      action->TakeEffect(cvc_);

      // spit out the action vector:
      LogAction(action);
    }

    // whether the action is valid or not, we need to save the experience to
    // learn from, to maintain the contract with the Agent

  }
  //actions evaluated, so dump the list
  queued_actions_.clear();

  if (action_log_) {
    fflush(action_log_);
  }

  //convenient place to score all the characters
  for (Agent* agent : agents_) {
    agent->GetCharacter()->SetScore(agent->Score(cvc_));
  }

}

void DecisionEngine::ChooseActions() {
  // go through list of all characters
  for (Agent* agent : agents_) {
    Action* a = agent->ChooseAction(cvc_);
    queued_actions_.push_back(a);
  }
}

void DecisionEngine::Learn() {
  for (Agent* agent : agents_) {
    agent->Learn(cvc_);
  }
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

    fprintf(action_log_, "%d\t%d\t%f\t%s\t%f\t%f\t%s\n", cvc_->Now(),
            action->GetActor()->GetId(), action->GetActor()->GetMoney(),
            action->GetActionId(), action->GetReward(), action->GetScore(), s.str().c_str());
  }
}
