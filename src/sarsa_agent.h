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

template <int N>
class SARSALearner;

template <int N>
class ExperienceImpl : public Experience {
 public:
  ExperienceImpl(std::unique_ptr<Action> action, double score,
                 Experience* next_experience, double features[N],
                 SARSALearner<N>* learner)
      : Experience(std::move(action), score, next_experience),
        learner_(learner) {
          for(int i=0; i<N; i++) {
            features_[i] = features[i];
          }
        }

  double features_[N];
  SARSALearner<N>* learner_;

  double Learn(CVC* cvc) override;

  double PredictScore() const override {
    return learner_->Score(features_);
  }
};

template <int N>
class SARSALearner {
 public:
  static void ReadWeights(
      const char* weight_file,
      std::unordered_map<std::string, SARSALearner*> learners);

  static void WriteWeights(
      const char* weight_file,
      std::unordered_map<std::string, SARSALearner*> learners);


  //creates a randomly initialized learner
  static std::unique_ptr<SARSALearner> Create(int learner_id, double n,
                                              double g, double b1, double b2,
                                              std::mt19937& random_generator,
                                              Logger* learn_logger);

  SARSALearner(int learner_id, double n, double g, double b1, double b2,
               double weights[N], Stats s[N],
               double m[N],  double r[N],
               Logger* learn_logger)
    : learner_id_(learner_id),
      n_(n),
      g_(g),
      b1_(b1),
      b2_(b2),
      learn_logger_(learn_logger) {
    for(int i=0; i<N; i++) {
      weights_[i] = weights[i];
      feature_stats_[i] = s[i];
      m_[i] = m[i];
      r_[i] = r[i];
    }
  }

  double Learn(CVC* cvc, ExperienceImpl<N>* experience);

  void WriteWeights(FILE* weights_file);
  void ReadWeights(FILE* weights_file);

  double Score(const double features[N]) const;
  double ComputeDiscountedRewards(const Experience* experience) const;

  std::unique_ptr<Experience> WrapAction(double features[N],
                                         std::unique_ptr<Action> action) {
    action->SetScore(Score(features));
    return std::make_unique<ExperienceImpl<N>>(std::move(action), 0.0, nullptr,
                                               features, this);
  }

 private:
  int learner_id_;

  double n_; //learning rate
  double g_; //discount factor
  double weights_[N];

  Stats feature_stats_[N];

  //adam optimizer params and state
  double b1_;
  double b2_;
  //TODO: parametrize ADAM epsilon?
  double epsilon_ = .000000001; //10^-8
  //TODO: setting learning epoch always to 0 means we can't load state
  //so this would need to get serialized out
  int t_=0;
  double m_[N];
  double r_[N];

  Logger* learn_logger_;
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
