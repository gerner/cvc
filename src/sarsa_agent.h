#ifndef SARSA_AGENT_H_
#define SARSA_AGENT_H_

#include <vector>
#include <unordered_map>
#include <random>
#include <memory>
#include <set>

#include "action.h"
#include "decision_engine.h"

class SARSALearner;

struct Experience {
  Experience(std::unique_ptr<Action>&& action, double score, double reward,
             Action* next_action, SARSALearner* learner)
      : action_(std::move(action)),
        score_(score),
        reward_(reward),
        next_action_(next_action),
        learner_(learner) {}

  std::unique_ptr<Action> action_; //the action we took (or will take)
  double score_; //score at the time we chose the action
  double reward_; //the reward from taking this action
  // TODO: do we really need the next action? or is it sufficient to simply have
  // a prediction of the future score from here?
  Action* next_action_; //the next action we'll take

  SARSALearner* learner_;
};

class SARSALearner {
 public:
  static void ReadWeights(
      const char* weight_file,
      std::unordered_map<std::string, SARSALearner*> learners);

  static void WriteWeights(
      const char* weight_file,
      std::unordered_map<std::string, SARSALearner*> learners);

  //creates a randomly initialized learner
  static std::unique_ptr<SARSALearner> Create(double n, double g,
                                              std::mt19937& random_generator,
                                              size_t num_features,
                                              Logger* learn_logger);

  SARSALearner(double n, double g, std::vector<double> weights,
               Logger* learn_logger);

  void Learn(CVC* cvc, Experience* experience);

  void WriteWeights(FILE* weights_file);
  void ReadWeights(FILE* weights_file);

  double Score(const std::vector<double>& features);

  std::unique_ptr<Experience> WrapAction(std::unique_ptr<Action> action) {
    return std::make_unique<Experience>(std::move(action), 0.0, 0.0, nullptr,
                                        this);
  }

 private:
  double n_; //learning rate
  double g_; //discount factor
  std::vector<double> weights_;

  Logger* learn_logger_;
};

// just one kind of action, one model
class SARSAActionFactory {
 public:
  virtual ~SARSAActionFactory() {}

  SARSAActionFactory(std::unique_ptr<SARSALearner> learner);

  SARSALearner* GetLearner() { return learner_.get(); }

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;

 protected:
  std::unique_ptr<SARSALearner> learner_;
};

// just one kind of response, one model
class SARSAResponseFactory {
 public:
  SARSALearner* GetLearner() { return learner_.get(); }

  SARSAResponseFactory(std::unique_ptr<SARSALearner> learner)
      : learner_(std::move(learner)) {}

  virtual double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;
 protected:
  std::unique_ptr<SARSALearner> learner_;
};

class SARSAActionPolicy {
  public:
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
             SARSAActionPolicy* policy)
      : Agent(character),
        action_factories_(action_factories),
        response_factories_(response_factories),
        policy_(policy) {}

  Action* ChooseAction(CVC* cvc) override;

  Action* Respond(CVC* cvc, Action* action) override;

  void Learn(CVC* cvc) override;

 private:

  double Score(CVC* cvc);

  std::vector<SARSAActionFactory*> action_factories_;
  std::unordered_map<std::string, std::set<SARSAResponseFactory*>>
      response_factories_;
  SARSAActionPolicy* policy_;

  std::unique_ptr<Experience> next_action_ = nullptr;
  std::vector<std::unique_ptr<Experience>> experiences_;
};

#endif
