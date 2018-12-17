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

class TestAgent : public Agent {
 public:
  TestAgent(Character* c) : Agent(c) {}

  std::unique_ptr<Action> ChooseAction(CVC* cvc) override {
    choose_calls_++;
    return std::make_unique<RecordingTestAction>(character_, &tas_);
  }

  std::unique_ptr<Action> Respond(CVC* cvc, Action* action) override {
    return nullptr;
  }

  void Learn(CVC* cvc, std::unique_ptr<Experience> experience) override {
    EXPECT_NE(nullptr, cvc);
    EXPECT_NE(nullptr, experience->action_);
    EXPECT_NE(nullptr, experience->next_action_);

    EXPECT_STREQ("RTA", experience->action_->GetActionId());
    learn_calls_++;
  }

  int choose_calls_ = 0;
  int learn_calls_ = 0;
  TestActionState tas_;
};

class DecisionEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {

    decision_engine_ = DecisionEngine::Create({&a_}, &cvc_, nullptr);
  }

  std::mt19937 random_generator_;
  Character c_ = Character(0, 0.0);
  CVC cvc_ = CVC({&c_}, nullptr, random_generator_);

  TestAgent a_ = TestAgent(&c_);

  std::unique_ptr<DecisionEngine> decision_engine_;
};

TEST_F(DecisionEngineTest, TestRunOneGameLoop) {
  //test guarantees of RunOneGameLoop: actions evaluate, new actions are
  //chosen and game state has ticked forward
  int start_tick = cvc_.Now();

  //run first loop with empty state
  //no actions should happen
  //game should tick forward
  //new actions should be chosen
  //no learning should happen
  decision_engine_->RunOneGameLoop();

  EXPECT_EQ(start_tick + 1, cvc_.Now());
  EXPECT_EQ(0, a_.tas_.effects_);
  EXPECT_EQ(1, a_.choose_calls_);
  EXPECT_EQ(0, a_.learn_calls_);

  //check that new actions have been chosen

  //run another loop
  //the actions that got chose last time should play out now
  //we should have learned from the action that happened
  decision_engine_->RunOneGameLoop();

  EXPECT_EQ(start_tick + 2, cvc_.Now());
  EXPECT_EQ(1, a_.tas_.effects_);
  EXPECT_EQ(cvc_.Now() - 1, a_.tas_.last_tick_);
  EXPECT_EQ(2, a_.choose_calls_);
  EXPECT_EQ(1, a_.learn_calls_);
}
