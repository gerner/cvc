#ifndef ACTION_H_
#define ACTION_H_

#include "core.h"

class CVC;
class Character;

class Action {
  public:
    Action(Character* actor, double score);
    virtual ~Action();
    //Get this action's actor (the character taking the action)
    Character* GetActor();
    void SetActor(Character* actor);

    //Get the score this action's been given
    //it's assumed this is normalized against alternative actions
    //I'm not thrilled with this detail which makes it make sense ONLY in the
    //context of some other instances
    double GetScore();
    void SetScore(double score);

    //Determine if this particular action is valid in the given gamestate by
    //the given character
    virtual bool IsValid(const CVC& gamestate) = 0;

    //Have this action take effect by the given character
    virtual void TakeEffect(CVC& gamestate) = 0;

  private:
    Character* actor_;
    double score_;
};

class TrivialAction : public Action {
  public:
    TrivialAction(Character *actor, double score);

    //implementation of Action
    bool IsValid(const CVC& gamestate);
    void TakeEffect(CVC& gamestate);

};

//AskAction: ask target character for money, they accept with some chance increasing with opinion
class AskAction : public Action {
  public:
    AskAction(Character *actor, double score);

    //implementation of Action
    bool IsValid(const CVC& gamestate);
    void TakeEffect(CVC& gamestate);

  private:
    Character* target_;
    double request_amount_;
};

//StealAction: (try to) steal money from target character, succeeds with chance increasing with opinion, detected with chance increasing with opinion
class StealAction : public Action {
  public:
    StealAction(Character *actor, double score);

    //implementation of Action
    bool IsValid(const CVC& gamestate);
    void TakeEffect(CVC& gamestate);

  private:
    Character* target_;
    double steal_amount_;
};

//GiveAction: give money to target character, increases opinion depending on other opinion modifiers
class GiveAction : public Action {
  public:
    GiveAction(Character *actor, double score);

    //implementation of Action
    bool IsValid(const CVC& gamestate);
    void TakeEffect(CVC& gamestate);

  private:
    Character* target_;
    double gift_amount_;
};

#endif
