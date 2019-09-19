#ifndef SARSA_AGENT_H_
#define SARSA_AGENT_H_

#include <vector>
#include <unordered_map>
#include <random>
#include <memory>
#include <set>
#include <deque>
#include <cassert>

#include "../util.h"
#include "../core.h"
#include "../action.h"
#include "../decision_engine.h"

namespace cvc::sarsa {

class Experience {
 public:
  Experience(std::unique_ptr<Action>&& action, double score,
             Experience* next_experience)
      : action_(std::move(action)),
        score_(score),
        next_experience_(next_experience) {}

  virtual ~Experience() {}

  std::unique_ptr<Action> action_; //the action we took (or will take)
  double score_; //score at the time we chose the action
  // TODO: do we really need the next action? or is it sufficient to simply have
  // a prediction of the future score from here?
  Experience* next_experience_; //the next action we'll take

  virtual double Learn(CVC* cvc) = 0;
  virtual double PredictScore() const = 0;
};


class SARSAActionPolicy {
  public:
   virtual void UpdateGrad(double dL_dy, double y) {}

   virtual std::unique_ptr<Experience> ChooseAction(
       std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
       Character* character) = 0;

};

class ActionFactory {
 public:
  virtual ~ActionFactory() {}

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;
};

class ResponseFactory {
 public:
  virtual ~ResponseFactory() {}

  virtual double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;
};

//TODO: need to sort out exactly what abstraction the agent needs
// a learning agent
// configured with:
//  an action factory which can list (scored) candidate actions given current
//  game state
//  a respponse factory which can list (scored) candidate response actions given
//  current game state and some proposal action
//  a policy for choosing a single candidate action (or response) from a set of
//  candidates
//  a set of learners for different action experiences
template <class S>
class SARSAAgent : public Agent {
 public:
  SARSAAgent(S* scorer, Character* character,
             std::vector<ActionFactory*> action_factories,
             std::unordered_map<std::string, std::set<ResponseFactory*>>
                 response_factories,
             SARSAActionPolicy* policy, int n_steps)
      : Agent(character),
        action_factories_(action_factories),
        response_factories_(response_factories),
        policy_(policy),
        n_steps_(n_steps),
        scorer_(scorer) {
    // set up experience queue so the "current" set of experiences is an empty
    // list
    experience_queue_.push_back({});
  }

  Action* ChooseAction(CVC* cvc) override {
    //if we have what was previously the next action, stick it in the set of
    //experiences
    if(next_action_) {
      experience_queue_.front().push_back(std::move(next_action_));
    }

    // list the choices of actions
    std::vector<std::unique_ptr<Experience>> actions;

    double score = 0.0;
    for (ActionFactory* factory : action_factories_) {
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

  Action* Respond(CVC* cvc, Action* action) override {
    assert(action->GetActor() != character_);
    assert(action->GetTarget() == character_);

    //1. find the appropriate response factory
    assert(response_factories_.find(action->GetActionId()) !=
           response_factories_.end());

    //2. ask it to enumerate some (scored) responses
    std::vector<std::unique_ptr<Experience>> actions;

    double score = 0.0;
    for (ResponseFactory* factory :
         response_factories_[action->GetActionId()]) {
      score += factory->Respond(cvc, character_, action, &actions);
    }

    //3. choose
    experience_queue_.front().push_back(
        policy_->ChooseAction(&actions, cvc, character_));

    experience_queue_.front().back()->score_ = Score(cvc);
    return experience_queue_.front().back()->action_.get();
  }

  void Learn(CVC* cvc) override {
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

  double Score(CVC* cvc) override {
    return scorer_->Score(cvc, character_);

    //agent wants to make (and have) lots of money
    //return character_->GetMoney();

    //return -abs(10000 - character_->GetMoney());

    //agent wants to have more money than average
    //return character_->GetMoney() - cvc->GetMoneyStats().max_;

    //return character_->GetMoney() / cvc->GetMoneyStats().mean_;

    //agent wants everyone to like them at least a little
    //return cvc->GetOpinionOfStats(GetCharacter()->GetId()).min_;

    //agent wants to be liked more than average
    //return cvc->GetOpinionOfStats(GetCharacter()->GetId()).mean_ - cvc->GetOpinionStats().mean_;
  }

 private:

  std::vector<ActionFactory*> action_factories_;
  std::unordered_map<std::string, std::set<ResponseFactory*>>
      response_factories_;
  SARSAActionPolicy* policy_;

  std::unique_ptr<Experience> next_action_ = nullptr;
  size_t n_steps_ = 10;
  std::deque<std::vector<std::unique_ptr<Experience>>> experience_queue_;

  S *scorer_;
};

class MoneyScorer {
 public:
  double Score(CVC* cvc, Character* character) {
    return character->GetMoney();
  }
};

} //namespace cvc::sarsa

#endif
