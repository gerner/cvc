#ifndef ACTION_FACTORIES_H_
#define ACTION_FACTORIES_H_

#include "core.h"
#include "decision_engine.h"

class GiveActionFactory : public ActionFactory {
 public:
  double EnumerateActions(const CVC* cvc, Character* character,
                          std::vector<std::unique_ptr<Action>>* actions);
};

class AskActionFactory : public ActionFactory {
 public:
  double EnumerateActions(const CVC* cvc, Character* character,
                          std::vector<std::unique_ptr<Action>>* actions);
};

class TrivialActionFactory : public ActionFactory {
 public:
  double EnumerateActions(const CVC* cvc, Character* character,
                          std::vector<std::unique_ptr<Action>>* actions);
};

#endif
