#include <random>

#include "gtest/gtest.h"
#include "../src/action.h"
#include "../src/decision_engine.h"

struct TestActionState {
  int effects_ = 0;
  int last_tick_ = -1;
};

class RecordingTestAction : public Action {
 public:
  RecordingTestAction(Character* actor, TestActionState* tas)
      : Action("RTA", actor, 1.0, {}), tas_(tas) {}

  bool IsValid(const CVC* gamestate) {
    return true;
  }

  void TakeEffect(CVC* gamestate) {
    tas_->effects_++;
    tas_->last_tick_ = gamestate->Now();
  }

  TestActionState* tas_;
};

class RecordingTestActionFactory : public ActionFactory {
 public:
  double EnumerateActions(CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Action>>* actions) override {
    actions->push_back(std::make_unique<RecordingTestAction>(character, &tas_));
    enumerate_calls_++;
    return 0;
  }

  void Learn(CVC* cvc, const Action* action,
             const Action* next_action) override {
    learn_calls_++;
  }

  int enumerate_calls_ = 0;
  int learn_calls_ = 0;
  TestActionState tas_;
};

class RecordingTestActionPolicy : public ActionPolicy {
 public:
  std::unique_ptr<Action> ChooseAction(
       std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
       Character* character) {
    choose_calls_++;
    return std::move(actions->front());
  }

  int choose_calls_ = 0;
};

class DecisionEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // TODO: prepare decsion engine
    // set up character(s)
    // set up agent(s)
    // set up CVC

    decision_engine_ = DecisionEngine::Create({&a_}, &cvc_, nullptr);
  }

  std::mt19937 random_generator_;
  Character c_ = Character(0, 0.0);
  CVC cvc_ = CVC({&c_}, nullptr, random_generator_);

  RecordingTestActionFactory rtaf_;
  RecordingTestActionPolicy policy_;

  Agent a_ = Agent(&c_, &rtaf_, &policy_);

  std::unique_ptr<DecisionEngine> decision_engine_;
};

TEST_F(DecisionEngineTest, TestRunOneGameLoop) {
  //TODO: test guarantees of RunOneGameLoop: actions evaluate, new actions are
  //chosen and game state has ticked forward
  int start_tick = cvc_.Now();

  //run first loop with empty state
  //no actions should happen
  //game should tick forward
  //new actions should be chosen
  //no learning should happen
  decision_engine_->RunOneGameLoop();

  EXPECT_EQ(start_tick + 1, cvc_.Now());
  EXPECT_EQ(0, rtaf_.tas_.effects_);
  EXPECT_EQ(1, rtaf_.enumerate_calls_);
  EXPECT_EQ(0, rtaf_.learn_calls_);

  //TODO: check that new actions have been chosen

  //run another loop
  //the actions that got chose last time should play out now
  //we should have learned from the action that happened
  decision_engine_->RunOneGameLoop();

  EXPECT_EQ(start_tick + 2, cvc_.Now());
  EXPECT_EQ(1, rtaf_.tas_.effects_);
  EXPECT_EQ(cvc_.Now() - 1, rtaf_.tas_.last_tick_);
  EXPECT_EQ(2, rtaf_.enumerate_calls_);
  EXPECT_EQ(1, rtaf_.learn_calls_);
}
