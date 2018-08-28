#ifndef CORE_H_
#define CORE_H_

#include <vector>
#include <memory>

class Character {
  public:
    double GetMoney();
    void SetMoney(double money);

  private:
    double money_;
};

class CVC;

class Action {
  public:
    //Get this action's actor (the character taking the action)
    Character* GetActor();
    //Get the score this action's been given
    //it's assumed this is normalized against alternative actions
    //I'm not thrilled with this detail which makes it make sense ONLY in the
    //context of some other instances
    double GetScore();

    //Determine if this particular action is valid in the given gamestate by
    //the given character
    virtual bool IsValid(CVC* gamestate) = 0;

    //Have this action take effect by the given character
    virtual void TakeEffect(CVC* gamestate) = 0;

  private:
    Character* actor_;
    double score_;
};

class DecisionEngine {
  public:
    //TODO: what's the right return type here? the idea is that this is a
    //factory of scored actions in a vector
};

class CVC {
  public:
    void GameLoop();

  private:
    void EvaluateQueuedActions();
    void ChooseActions();

    std::vector<std::unique_ptr<Character>> characters_;
    std::vector<std::unique_ptr<Action>> queued_actions_;

};

#endif
