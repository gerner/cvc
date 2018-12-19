#ifndef ACTION_FACTORIES_H_
#define ACTION_FACTORIES_H_

#include <unordered_map>
#include <vector>
#include <string>

#include "core.h"
#include "decision_engine.h"

// Creates and scores action instances for a specific type of action
class ActionFactory {
 public:
  virtual ~ActionFactory() {}

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) = 0;

};

class ResponseFactory {
 public:
  virtual ~ResponseFactory() {}
  virtual double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Action>>* actions) = 0;
};

class ActionPolicy {
  public:
   virtual std::unique_ptr<Action> ChooseAction(
       std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
       Character* character) = 0;
};

class HeuristicAgent : public Agent {
 public:
  HeuristicAgent(Character* character, ActionFactory* action_factory,
                 ResponseFactory* response_factory, ActionPolicy* policy)
      : Agent(character),
        action_factory_(action_factory),
        response_factory_(response_factory),
        policy_(policy) {}

  Action* ChooseAction(CVC* cvc) override {
    // list the choices of actions
    std::vector<std::unique_ptr<Action>> actions;
    action_factory_->EnumerateActions(cvc, character_, &actions);

    // choose one according to the policy and store it, along with this agent in
    // a partial Action which we will fill out later
    next_action_ = policy_->ChooseAction(&actions, cvc, character_);
    return next_action_.get();
  }

  Action* Respond(CVC* cvc, Action* action) override {
     //TODO: heuristic response TBD
     responses_.push_back(std::make_unique<TrivialAction>(character_, 0.0,
                                         std::vector<double>({})));
     return responses_.back().get();
  }

  // no learning on the heuristic agent
  void Learn(CVC* cvc) override {
    responses_.clear();
  }

 private:
  ActionFactory* action_factory_;
  ResponseFactory* response_factory_;
  ActionPolicy* policy_;

  std::vector<std::unique_ptr<Action>> responses_;
  std::unique_ptr<Action> next_action_;
};

class GiveActionFactory : public ActionFactory {
 public:
  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class AskActionFactory : public ActionFactory {
 public:
  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class AskResponseFactory : public ResponseFactory {
 public:
  double Respond(CVC* cvc, Character* character, Action* action,
                 std::vector<std::unique_ptr<Action>>* responses) override;
};

class WorkActionFactory : public ActionFactory {
 public:
  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class TrivialActionFactory : public ActionFactory {
 public:
  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;
};

class CompositeActionFactory : public ActionFactory {
 public:
  CompositeActionFactory(
      std::unordered_map<std::string, ActionFactory*> factories);

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override;

 private:
  std::unordered_map<std::string, ActionFactory*> factories_;
};

class ProbDistPolicy : public ActionPolicy {
 public:
  std::unique_ptr<Action> ChooseAction(
      std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
      Character* character) override;
};

#endif
