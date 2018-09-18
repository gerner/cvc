#ifndef ACTION_H_
#define ACTION_H_

#include "core.h"
#include "decision_engine.h"

class TrivialAction : public Action {
 public:
  TrivialAction(Character* actor, double score, std::vector<double> features);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);
};

// AskAction: ask target character for money, they accept with some chance
// increasing with opinion
class AskAction : public Action {
 public:
  AskAction(Character* actor, double score, std::vector<double> features,
            Character* target, double request_amount);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  Character* target_;
  double request_amount_;
};

// StealAction: (try to) steal money from target character, succeeds with chance
// increasing with opinion, detected with chance increasing with opinion
class StealAction : public Action {
 public:
  StealAction(Character* actor, double score, std::vector<double> features);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  Character* target_;
  double steal_amount_;
};

// GiveAction: give money to target character, increases opinion depending on
// other opinion modifiers
class GiveAction : public Action {
 public:
  GiveAction(Character* actor, double score, std::vector<double> features,
             Character* target, double gift_amount);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  Character* target_;
  double gift_amount_;
};

#endif
