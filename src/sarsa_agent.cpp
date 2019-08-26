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

Action* SARSAAgent::ChooseAction(CVC* cvc) {
  //if we have what was previously the next action, stick it in the set of
  //experiences
  if(next_action_) {
    experience_queue_.front().push_back(std::move(next_action_));
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
  //keep track of the current score at the time this action was chosen
  next_action_->score_ = Score(cvc);

  return next_action_->action_.get();
}

Action* SARSAAgent::Respond(CVC* cvc, Action* action) {
  assert(action->GetActor() != character_);
  assert(action->GetTarget() == character_);

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
  experience_queue_.front().push_back(
      policy_->ChooseAction(&actions, cvc, character_));

  experience_queue_.front().back()->score_ = Score(cvc);
  return experience_queue_.front().back()->action_.get();
}

void SARSAAgent::Learn(CVC* cvc) {
  // we've just wrapped up a turn, so now is a good time to incorporate
  // information about the reward we received this turn.
  // 1. update rewards for all experiences (the learner does this when it
  // learns)

  //set the next experience pointer on the latest experiences
  for(auto& experience : experience_queue_.front()) {
    experience->next_experience_ = next_action_.get();
  }

  // 2. learn if necessary
  // n-step SARSA
  if(experience_queue_.size() == n_steps_) {
    // we learn from all of the experiences n_steps ago
    // recall, multiple experiences might happen at the same step
    // because, e.g. response actions that resolve in the same tick
    for(auto& experience : experience_queue_.back()) {
      double dL_dy = experience->Learn(cvc);
      // TODO: do we need to worry about GetScore returning a stale score, which
      // wasn't used to product dL_dy?
      // TODO: why have this at all?
      //policy_->UpdateGrad(dL_dy, experience->action_->GetScore());
    }
    //toss the experiences from which we just learned (at the back)
    experience_queue_.pop_back();
  }

  //set up space for the next batch of experiences (for the upcoming turn)
  experience_queue_.push_front(std::vector<std::unique_ptr<Experience>>());
}

double SARSAAgent::Score(CVC* cvc) {
  //TODO: maybe abstract this?
  //agent wants to make (and have) lots of money
  return character_->GetMoney();

  //return -abs(10000 - character_->GetMoney());

  //agent wants to have more money than average
  //return character_->GetMoney() - cvc->GetMoneyStats().max_;

  //return character_->GetMoney() / cvc->GetMoneyStats().mean_;

  //agent wants everyone to like them at least a little
  //return cvc->GetOpinionOfStats(GetCharacter()->GetId()).min_;

  //agent wants to be liked more than average
  //return cvc->GetOpinionOfStats(GetCharacter()->GetId()).mean_ - cvc->GetOpinionStats().mean_;
}
