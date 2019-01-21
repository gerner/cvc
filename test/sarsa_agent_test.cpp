#include <random>

#include "gtest/gtest.h"
#include "../src/action.h"
#include "../src/sarsa_agent.h"

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

class SarsaAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    learner_ =
        SARSALearner::Create(0.001, 0.8, random_generator_, 2, &learn_logger_);
  }

  Logger learn_logger_;
  std::mt19937 random_generator_;
  std::unique_ptr<SARSALearner> learner_;
};

TEST_F(SarsaAgentTest, TestExperienceRewards) {
  // set up sequence of experiences and make sure the discounted rewards and
  // future score estimate are computed properly

  Experience e4(std::make_unique<RecordingTestAction>(nullptr, nullptr), 10.0, nullptr, nullptr);
  e4.action_->SetScore(3.7);
  Experience e3(nullptr, 5.0, &e4, nullptr);
  Experience e2(nullptr, 2.5, &e3, nullptr);
  Experience e1(nullptr, 0.0, &e2, nullptr);
  //rewards:
  //e1 = 2.5
  //e2 = 2.5
  //e3 = 5.0
  //
  //fully discounted rewards: 2.5 + 0.8 * 2.5 + 0.8^2 * 5.0 + 0.8^3 3.7

  double expected_rewards = 2.5 + 0.8 * 2.5 + 0.8*0.8*5.0 + 0.8*0.8*0.8*3.7;
  double rewards = learner_->ComputeDiscountedRewards(&e1);
  EXPECT_DOUBLE_EQ(expected_rewards, rewards);
}
