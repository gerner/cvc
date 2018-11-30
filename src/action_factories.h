#ifndef ACTION_FACTORIES_H_
#define ACTION_FACTORIES_H_

#include <unordered_map>
#include <vector>
#include <string>

#include "core.h"
#include "decision_engine.h"

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

  void Learn(CVC* cvc, const Action* action,
             const Action* next_action) override;

 private:
  std::unordered_map<std::string, ActionFactory*> factories_;
};

class ProbDistPolicy : public ActionPolicy {
 public:
  std::unique_ptr<Action> ChooseAction(std::vector<std::unique_ptr<Action>>* actions, CVC* cvc, Character* character) override;
};

#endif
