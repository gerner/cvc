#include <random>

#include "gtest/gtest.h"
#include "../src/util.h"
#include "../src/core.h"
#include "../src/action.h"
#include "../src/sarsa/sarsa_agent.h"
#include "../src/sarsa/sarsa_learner.h"

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
    random_generator_ = std::mt19937(rd());
    learner_ = cvc::sarsa::SARSALearner<1>::Create(
        1, 0.001, 0.8, 0.0, 0.0, random_generator_, &learn_logger_);
  }

  Logger learn_logger_;
  std::random_device rd;
  std::mt19937 random_generator_;
  std::unique_ptr<cvc::sarsa::SARSALearner<1>> learner_;
};

TEST_F(SarsaAgentTest, TestExperienceRewards) {
  // set up sequence of experiences and make sure the discounted rewards and
  // future score estimate are computed properly

  std::array<double, 1> zero_array = {0.0};
  cvc::sarsa::ExperienceImpl<1> e4(
      std::make_unique<RecordingTestActionSAT>(nullptr, nullptr), 10.0, nullptr,
      zero_array, learner_.get());
  e4.action_->SetScore(3.7);
  cvc::sarsa::ExperienceImpl<1> e3(nullptr, 5.0, &e4, zero_array,
                                   learner_.get());
  cvc::sarsa::ExperienceImpl<1> e2(nullptr, 2.5, &e3, zero_array,
                                   learner_.get());
  cvc::sarsa::ExperienceImpl<1> e1(nullptr, 0.0, &e2, zero_array,
                                   learner_.get());
  //rewards:
  //e1 = 2.5
  //e2 = 2.5
  //e3 = 5.0
  //
  //fully discounted rewards: 2.5 + 0.8 * 2.5 + 0.8^2 * 5.0 + 0.8^3 3.7

  double expected_rewards = 2.5 + 0.8 * 2.5 + 0.8 * 0.8 * 5.0 +
                            0.8 * 0.8 * 0.8 * learner_->Score(zero_array);
  double rewards = learner_->ComputeDiscountedRewards(&e1);
  EXPECT_DOUBLE_EQ(expected_rewards, rewards);
}

TEST_F(SarsaAgentTest, TestLearningMonotonic) {
  // test that learning gets better (which it will in a very simple linear case)
  CVC cvc;
  std::array<double, 1> one_array = {1.0};
  cvc::sarsa::ExperienceImpl<1> e2(
      std::make_unique<RecordingTestActionSAT>(nullptr, nullptr), 10.0, nullptr,
      one_array, learner_.get());
  cvc::sarsa::ExperienceImpl<1> e1(
      std::make_unique<RecordingTestActionSAT>(nullptr, nullptr), 0.0, &e2,
      one_array, learner_.get());

  Logger logger;

  double truth_estimate = learner_->ComputeDiscountedRewards(&e1);
  double estimated_score = learner_->Score(e1.features_);
  double first_loss = pow(estimated_score - truth_estimate, 2);

  double dL_dy = e1.Learn(&cvc);


  truth_estimate = learner_->ComputeDiscountedRewards(&e1);
  estimated_score = learner_->Score(e1.features_);
  double second_loss = pow(estimated_score - truth_estimate, 2);

  logger.Log(INFO, "first loss: %f first gradient: %f second loss: %f\n", first_loss,
              dL_dy, second_loss);
  EXPECT_LT(second_loss, first_loss);
}
