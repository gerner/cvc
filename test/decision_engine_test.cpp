#include <random>

#include "gtest/gtest.h"
#include "../src/action.h"
#include "../src/decision_engine.h"

struct TestActionState {
  int effects_ = 0;
  int last_tick_ = -1;
};

class RecordingTestActionDET : public Action {
 public:
  RecordingTestActionDET(Character* actor, TestActionState* tas)
      : Action("RTA", actor, 1.0), tas_(tas) {}

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

  Action* ChooseAction(CVC* cvc) override {
    choose_calls_++;
    next_action_ = std::make_unique<RecordingTestActionDET>(character_, &tas_);
    return next_action_.get();
  }

  Action* Respond(CVC* cvc, Action* action) override {
    return nullptr;
  }

  void Learn(CVC* cvc) override {
    EXPECT_NE(nullptr, cvc);
    learn_calls_++;
  }

  double Score(CVC* cvc) override {
    return 0.0;
  }

  std::unique_ptr<Action> next_action_ = nullptr;

  int choose_calls_ = 0;
  int learn_calls_ = 0;
  TestActionState tas_;
};

class DecisionEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    logger_.SetLogLevel(WARN);
    decision_engine_ = DecisionEngine::Create({&a_}, &cvc_, &logger_);
  }

  std::mt19937 random_generator_;
  Character c_ = Character(0, 0.0);
  CVC cvc_ = CVC({&c_}, nullptr, random_generator_);

  TestAgent a_ = TestAgent(&c_);

  Logger logger_;
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
  //we should get a learn call
  decision_engine_->RunOneGameLoop();

  EXPECT_EQ(start_tick + 1, cvc_.Now());
  EXPECT_EQ(0, a_.tas_.effects_);
  EXPECT_EQ(1, a_.choose_calls_);
  EXPECT_EQ(1, a_.learn_calls_);

  //check that new actions have been chosen

  //run another loop
  //the actions that got chose last time should play out now
  //we should have learned from the action that happened
  decision_engine_->RunOneGameLoop();

  EXPECT_EQ(start_tick + 2, cvc_.Now());
  EXPECT_EQ(1, a_.tas_.effects_);
  EXPECT_EQ(cvc_.Now() - 1, a_.tas_.last_tick_);
  EXPECT_EQ(2, a_.choose_calls_);
  EXPECT_EQ(2, a_.learn_calls_);
}

