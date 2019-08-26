#ifndef ACTION_H_
#define ACTION_H_

#include <vector>

#include "core.h"

class Action {
 public:
  Action(const char* action_id, Character* actor, double score);
  Action(const char* action_id, Character* actor, Character* target, double score);
  virtual ~Action();
  // Get this action's actor (the character taking the action)
  Character* GetActor() const {
    return actor_;
  }
  void SetActor(Character* actor) {
    actor_ = actor;
  }

  Character* GetTarget() const {
    return target_;
  }
  void SetTarget(Character* target) {
    target_ = target;
  }

  // Get the score this action's been given
  // it's assumed this is normalized against alternative actions
  // I'm not thrilled with this detail which makes it make sense ONLY in the
  // context of some other instances
  double GetScore() const {
    return score_;
  }
  void SetScore(double score) {
    score_ = score;
  }

  const char* GetActionId() const {
    return action_id_;
  }

  // Determine if this particular action is valid in the given gamestate by
  // the given character
  virtual bool IsValid(const CVC* gamestate) = 0;

  virtual bool RequiresResponse();

  // Have this action take effect by the given character
  virtual void TakeEffect(CVC* gamestate) = 0;

 private:
  const char* action_id_;
  Character* actor_;
  Character* target_;
  double score_;
};

class TrivialAction : public Action {
 public:
  TrivialAction(Character* actor, double score);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);
};

class TrivialResponse : public Action {
 public:
  TrivialResponse(Character* actor, double score);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);
};

class WorkAction : public Action {
 public:
  WorkAction(Character* actor, double score);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);
};

// AskAction: ask target character for money, they accept with some chance
// increasing with opinion
class AskAction : public Action {
 public:
  AskAction(Character* actor, double score, Character* target,
            double request_amount);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  bool RequiresResponse() override;
  void TakeEffect(CVC* gamestate);

  double GetRequestAmount() const {
    return request_amount_;
  }

 private:
  double request_amount_;
};

class AskSuccessAction : public Action {
 public:
  AskSuccessAction(Character* actor, double score, Character* target,
                   AskAction* source_action);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  AskAction* source_action_;
};

// StealAction: (try to) steal money from target character, succeeds with chance
// increasing with opinion, detected with chance increasing with opinion
class StealAction : public Action {
 public:
  StealAction(Character* actor, double score);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  double steal_amount_;
};

// GiveAction: give money to target character, increases opinion depending on
// other opinion modifiers
class GiveAction : public Action {
 public:
  GiveAction(Character* actor, double score, Character* target,
             double gift_amount);

  // implementation of Action
  bool IsValid(const CVC* gamestate);
  void TakeEffect(CVC* gamestate);

 private:
  double gift_amount_;
};

#endif
