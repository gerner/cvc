#include <cmath>

#include "gtest/gtest.h"
#include "../src/core.h"

TEST(StatsTest, TestComputeStats) {
  Stats s;
  double sum = 0.0;
  double ss = 0.0;

  sum += 0.0;
  ss += 0.0;
  sum += 1.0;
  ss += 1.0;
  sum += 2.0;
  ss += 2.0 * 2.0;

  s.ComputeStats(sum, ss, 3);

  EXPECT_EQ(3, s.n_);
  EXPECT_DOUBLE_EQ(1.0, s.mean_);
  EXPECT_NEAR(sqrt(0.666666666), s.stdev_, 0.0000001);
}

class CVCTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::mt19937 random_generator(rd());

    std::vector<Character*> c;

    characters.push_back(std::make_unique<Character>(0, 100));
    c.push_back(characters.back().get());
    c.back()->traits_[kBackground] = 0;
    characters.push_back(std::make_unique<Character>(1, 200));
    c.push_back(characters.back().get());
    c.back()->traits_[kBackground] = 0;

    cvc = CVC(c, &logger, random_generator);
  }

  std::random_device rd;
  Logger logger;

  std::vector<std::unique_ptr<Character>> characters;
  CVC cvc;
};

TEST_F(CVCTest, TestTick) {
  int now = cvc.Now();
  EXPECT_EQ(0, now);
  EXPECT_EQ(now, cvc.Now());

  cvc.Tick();
  EXPECT_EQ(now+1, cvc.Now());
}

TEST_F(CVCTest, TestMoneyStats) {
  EXPECT_EQ(characters.size(), cvc.GetMoneyStats().n_);
  EXPECT_DOUBLE_EQ(150.0, cvc.GetMoneyStats().mean_);

  characters[0]->SetMoney(300);
  cvc.Tick();


  EXPECT_DOUBLE_EQ(250.0, cvc.GetMoneyStats().mean_);
}

TEST_F(CVCTest, TestOpinionStats) {
  EXPECT_EQ(characters.size(), cvc.GetOpinionStats().n_);
  EXPECT_DOUBLE_EQ(25.0, cvc.GetOpinionStats().mean_);

  EXPECT_EQ(characters.size()-1, cvc.GetOpinionOfStats(0).n_);
  EXPECT_DOUBLE_EQ(25.0, cvc.GetOpinionOfStats(0).mean_);
  EXPECT_EQ(characters.size()-1, cvc.GetOpinionOfStats(1).n_);
  EXPECT_DOUBLE_EQ(25.0, cvc.GetOpinionOfStats(1).mean_);

  EXPECT_EQ(characters.size()-1, cvc.GetOpinionByStats(0).n_);
  EXPECT_DOUBLE_EQ(25.0, cvc.GetOpinionByStats(0).mean_);
  EXPECT_EQ(characters.size()-1, cvc.GetOpinionByStats(1).n_);
  EXPECT_DOUBLE_EQ(25.0, cvc.GetOpinionByStats(1).mean_);

  //now get rid of the common background and tick the game
  characters[0]->traits_.erase(characters[0]->traits_.find(kBackground));
  cvc.Tick();

  EXPECT_EQ(characters.size(), cvc.GetOpinionStats().n_);
  EXPECT_DOUBLE_EQ(0.0, cvc.GetOpinionStats().mean_);

  EXPECT_EQ(characters.size()-1, cvc.GetOpinionOfStats(0).n_);
  EXPECT_DOUBLE_EQ(0.0, cvc.GetOpinionOfStats(0).mean_);
  EXPECT_EQ(characters.size()-1, cvc.GetOpinionOfStats(1).n_);
  EXPECT_DOUBLE_EQ(0.0, cvc.GetOpinionOfStats(1).mean_);

  EXPECT_EQ(characters.size()-1, cvc.GetOpinionByStats(0).n_);
  EXPECT_DOUBLE_EQ(0.0, cvc.GetOpinionByStats(0).mean_);
  EXPECT_EQ(characters.size()-1, cvc.GetOpinionByStats(1).n_);
  EXPECT_DOUBLE_EQ(0.0, cvc.GetOpinionByStats(1).mean_);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
