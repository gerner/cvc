#include <random>

#include "gtest/gtest.h"
#include "../src/action.h"
#include "../src/sarsa_agent.h"
#include "../src/sarsa_learner.h"

struct TestActionState {
  int effects_ = 0;
  int last_tick_ = -1;
};

class RecordingTestActionSAT : public Action {
 public:
  RecordingTestActionSAT(Character* actor, TestActionState* tas)
      : Action("RTA", actor, 1.0), tas_(tas) {
      }

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
        SARSALearner<0>::Create(1, 0.001, 0.8, 0.0, 0.0, random_generator_, &learn_logger_);
  }

  Logger learn_logger_;
  std::mt19937 random_generator_;
  std::unique_ptr<SARSALearner<0>> learner_;
};

TEST_F(SarsaAgentTest, TestExperienceRewards) {
  // set up sequence of experiences and make sure the discounted rewards and
  // future score estimate are computed properly

  ExperienceImpl<0> e4(std::make_unique<RecordingTestActionSAT>(nullptr, nullptr), 10.0, nullptr, {}, learner_.get());
  e4.action_->SetScore(3.7);
  ExperienceImpl<0> e3(nullptr, 5.0, &e4, {}, learner_.get());
  ExperienceImpl<0> e2(nullptr, 2.5, &e3, {}, learner_.get());
  ExperienceImpl<0> e1(nullptr, 0.0, &e2, {}, learner_.get());
  //rewards:
  //e1 = 2.5
  //e2 = 2.5
  //e3 = 5.0
  //
  //fully discounted rewards: 2.5 + 0.8 * 2.5 + 0.8^2 * 5.0 + 0.8^3 3.7

  double expected_rewards = 2.5 + 0.8 * 2.5 + 0.8*0.8*5.0 + 0.8*0.8*0.8*learner_->Score({});
  double rewards = learner_->ComputeDiscountedRewards(&e1);
  EXPECT_DOUBLE_EQ(expected_rewards, rewards);
}
