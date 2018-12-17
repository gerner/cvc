#ifndef SARSA_ACTION_FACTORIES_H_
#define SARSA_ACTION_FACTORIES_H_

#include <vector>
#include <unordered_map>
#include <random>
#include <memory>

#include "action.h"
#include "decision_engine.h"

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
  void Learn(CVC* cvc, std::unique_ptr<Experience> experience);

  void WriteWeights(FILE* weights_file);
  void ReadWeights(FILE* weights_file);

  double Score(const std::vector<double>& features);

 private:
  double n_; //learning rate
  double g_; //discount factor
  std::vector<double> weights_;

  Logger* learn_logger_;
};

class SARSAActionFactory : public ActionFactory {
 public:
  SARSAActionFactory(std::unique_ptr<SARSALearner> learner);

  SARSALearner* GetLearner() { return learner_.get(); }

 protected:
  std::unique_ptr<SARSALearner> learner_;
};

class SARSAResponseFactory : public ResponseFactory {
 public:
  virtual double Respond(CVC* cvc, Action* action,
                         std::vector<std::unique_ptr<Action>>* actions) = 0;

  SARSALearner* GetLearner() { return learner_.get(); }

 protected:
  std::unique_ptr<SARSALearner> learner_;
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
  SARSAAgent(Character* character, ActionFactory* action_factory,
             ResponseFactory* response_factory, ActionPolicy* policy,
             std::unordered_map<std::string, SARSALearner*> learners)
      : Agent(character),
        action_factory_(action_factory),
        response_factory_(response_factory),
        policy_(policy),
        learners_(learners) {}

  std::unique_ptr<Action> ChooseAction(CVC* cvc) override;
  std::unique_ptr<Action> Respond(CVC* cvc, Action* action) override;
  void Learn(CVC* cvc, std::unique_ptr<Experience> experience) override;

 private:
  ActionFactory* action_factory_;
  ResponseFactory* response_factory_;
  ActionPolicy* policy_;
  std::unordered_map<std::string, SARSALearner*> learners_;
};


class SARSAGiveActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAGiveActionFactory> Create(
      double n, double g, std::mt19937& random_generator, Logger* learn_logger);

  SARSAGiveActionFactory(std::unique_ptr<SARSALearner> learner);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSAAskActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAAskActionFactory> Create(
      double n, double g, std::mt19937& random_generator, Logger* learn_logger);

  SARSAAskActionFactory(std::unique_ptr<SARSALearner> learner);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSAWorkActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSAWorkActionFactory> Create(
      double n, double g, std::mt19937& random_generator, Logger* learn_logger);

  SARSAWorkActionFactory(std::unique_ptr<SARSALearner> learner);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSATrivialActionFactory : public SARSAActionFactory {
 public:
  static std::unique_ptr<SARSATrivialActionFactory> Create(
      double n, double g, std::mt19937& random_generator, Logger* learn_logger);

  SARSATrivialActionFactory(std::unique_ptr<SARSALearner> learner);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class SARSACompositeActionFactory : public ActionFactory {
 public:
  SARSACompositeActionFactory(
      std::unordered_map<std::string, SARSAActionFactory*> factories,
      const char* weight_file);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions);

 private:
  std::unordered_map<std::string, SARSAActionFactory*> factories_;
};

class EpsilonGreedyPolicy : public ActionPolicy {
 public:
  EpsilonGreedyPolicy(double epsilon) : epsilon_(epsilon){};

  std::unique_ptr<Action> ChooseAction(
      std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
      Character* character) override;

 private:
  double epsilon_;
};

#endif
