#ifndef SARSA_AGENT_H_
#define SARSA_AGENT_H_

#include <vector>
#include <unordered_map>
#include <random>
#include <memory>
#include <set>
#include <deque>

#include "util.h"
#include "core.h"
#include "action.h"
#include "decision_engine.h"

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


// just one kind of action, one model
class SARSAActionFactory {
 public:
  virtual ~SARSAActionFactory() {}

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;

 protected:
  double* StandardFeatures(CVC* cvc, Character* character,
                        double* features) const {
    features[0] = 1.0; //bias
    features[1] = log(character->GetMoney());
    features[2] = 0.0;//log(cvc->GetMoneyStats().mean_);
    features[3] = 0.0;//cvc->GetOpinionStats().mean_/100.0;
    features[4] = 0.0;//cvc->GetOpinionByStats(character->GetId()).mean_/100.0;
    features[5] = 0.0;//cvc->GetOpinionOfStats(character->GetId()).mean_/100.0;

    return features;
  }

  double* TargetFeatures(CVC* cvc, Character* character,
                                     Character* target,
                                     double* features) const {
    StandardFeatures(cvc, character, features);
    features[6] = 0.0;//character->GetOpinionOf(target) / 100.0;
    features[7] = 0.0;//target->GetOpinionOf(character) / 100.0;
    features[8] = log(target->GetMoney());
    //TODO: this should be relationship between character and target money
    features[9] = 1.0;

    return features;
  }

};

// just one kind of response, one model
class SARSAResponseFactory {
 public:

  virtual double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;
 protected:
  double* StandardFeatures(CVC* cvc, Character* character,
                        double* features) const {
    features[0] = 1.0; //bias
    features[1] = log(character->GetMoney());
    features[2] = 0.0;//log(cvc->GetMoneyStats().mean_);
    features[3] = 0.0;//cvc->GetOpinionStats().mean_/100.0;
    features[4] = 0.0;//cvc->GetOpinionByStats(character->GetId()).mean_/100.0;
    features[5] = 0.0;//cvc->GetOpinionOfStats(character->GetId()).mean_/100.0;

    return features;
  }

  double* TargetFeatures(CVC* cvc, Character* character,
                                     Character* target,
                                     double* features) const {
    StandardFeatures(cvc, character, features);
    features[6] = 0.0;//character->GetOpinionOf(target) / 100.0;
    features[7] = 0.0;//target->GetOpinionOf(character) / 100.0;
    features[8] = log(target->GetMoney());
    //TODO: this should be relationship between character and target money
    features[9] = 1.0;

    return features;
  }
};

class SARSAActionPolicy {
  public:
   virtual void UpdateGrad(double dL_dy, double y) {}

   virtual std::unique_ptr<Experience> ChooseAction(
       std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
       Character* character) = 0;

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
class SARSAAgent : public Agent {
 public:
  SARSAAgent(Character* character,
             std::vector<SARSAActionFactory*> action_factories,
             std::unordered_map<std::string, std::set<SARSAResponseFactory*>>
                 response_factories,
             SARSAActionPolicy* policy, int n_steps)
      : Agent(character),
        action_factories_(action_factories),
        response_factories_(response_factories),
        policy_(policy),
        n_steps_(n_steps) {
    // set up experience queue so the "current" set of experiences is an empty
    // list
    experience_queue_.push_back({});
  }

  Action* ChooseAction(CVC* cvc) override;

  Action* Respond(CVC* cvc, Action* action) override;

  void Learn(CVC* cvc) override;
  double Score(CVC* cvc) override;

 private:

  std::vector<SARSAActionFactory*> action_factories_;
  std::unordered_map<std::string, std::set<SARSAResponseFactory*>>
      response_factories_;
  SARSAActionPolicy* policy_;

  std::unique_ptr<Experience> next_action_ = nullptr;
  size_t n_steps_ = 10;
  std::deque<std::vector<std::unique_ptr<Experience>>> experience_queue_;
};
#endif
