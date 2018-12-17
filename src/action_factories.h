#ifndef ACTION_FACTORIES_H_
#define ACTION_FACTORIES_H_

#include <unordered_map>
#include <vector>
#include <string>

#include "core.h"
#include "decision_engine.h"

class HeuristicAgent : public Agent {
 public:
  HeuristicAgent(Character* character, ActionFactory* action_factory,
                 ResponseFactory* response_factory, ActionPolicy* policy)
      : Agent(character),
        action_factory_(action_factory),
        response_factory_(response_factory),
        policy_(policy) {}

  std::unique_ptr<Action> ChooseAction(CVC* cvc) override {
    // list the choices of actions
    std::vector<std::unique_ptr<Action>> actions;
    action_factory_->EnumerateActions(cvc, character_, &actions);

    // choose one according to the policy and store it, along with this agent in
    // a partial Experience which we will fill out later
    return policy_->ChooseAction(&actions, cvc, character_);
  }

  std::unique_ptr<Action> Respond(CVC* cvc, Action* action) override {
     //TODO: heuristic response TBD
     return nullptr;
  }

  // no learning on the heuristic agent
  void Learn(CVC* cvc, std::unique_ptr<Experience> experience) override {}

 private:
  ActionFactory* action_factory_;
  ResponseFactory* response_factory_;
  ActionPolicy* policy_;
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
