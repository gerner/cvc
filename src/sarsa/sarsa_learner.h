#ifndef SARSA_LEARNER_H
#define SARSA_LEARNER_H

#include <stdio.h>
#include <cassert>
#include <cmath>
#include <memory>

#include "../util.h"
#include "../core.h"
#include "sarsa_agent.h"

namespace cvc::sarsa {

template <size_t N>
class SARSALearner;

template <size_t N>
class ExperienceImpl : public Experience {
 public:
  ExperienceImpl(std::unique_ptr<Action> action, double score,
                 Experience* next_experience, double features[N],
                 SARSALearner<N>* learner)
      : Experience(std::move(action), score, next_experience),
        learner_(learner) {
          for(size_t i=0; i<N; i++) {
            features_[i] = features[i];
          }
        }

  double features_[N];
  SARSALearner<N>* learner_;

  double Learn(CVC* cvc) override {
    return learner_->Learn(cvc, this);
  }

  double PredictScore() const override {
    return learner_->Score(features_);
  }
};

template <size_t N>
class SARSALearner {
 public:
  static void ReadWeights(
      const char* weight_file,
      std::unordered_map<std::string, SARSALearner*> learners) {
    FILE* weights = fopen(weight_file, "r");

    if(!weights) {
      //if the file doesn't exist, just leave existing weights
      return;
    }

    size_t num_factories;
    int ret = fread(&num_factories, sizeof(size_t), 1, weights);
    assert(1 == ret);

    for(size_t i=0; i<num_factories; i++) {
      size_t name_length;
      ret = fread(&name_length, sizeof(size_t), 1, weights);
      assert(1 == ret);
      assert(1024-1 > name_length);
      char factory_name[1024];
      factory_name[name_length] = 0;
      ret = fread(factory_name, sizeof(char), name_length, weights);
      assert((size_t)ret == name_length);

      assert(learners.find(factory_name) != learners.end());
      learners[factory_name]->ReadWeights(weights);
    }
    fclose(weights);
  }

  static void WriteWeights(
      const char* weight_file,
      std::unordered_map<std::string, SARSALearner*> learners) {
    FILE* weights = fopen(weight_file, "w");
    assert(weights);

    size_t num_factories = learners.size();
    int ret = fwrite(&num_factories, sizeof(size_t), 1, weights);
    assert(ret);

    for(const auto& factory : learners) {
      size_t name_length = strlen(factory.first.c_str());
      ret = fwrite(&name_length, sizeof(size_t), 1, weights);
      assert(ret > 0);
      ret = fwrite(factory.first.c_str(), sizeof(char), name_length, weights);
      assert(name_length == (size_t)ret);
      factory.second->WriteWeights(weights);
    }
    fclose(weights);
  }

  //creates a randomly initialized learner
  static std::unique_ptr<SARSALearner> Create(int learner_id, double n,
                                              double g, double b1, double b2,
                                              std::mt19937& random_generator,
                                              Logger* learn_logger) {
    double weights[N];
    Stats stats[N];
    double m[N];
    double r[N];

    std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
    for (size_t i = 0; i < N; i++) {
      weights[i] = weight_dist(random_generator);
      m[i] = 0.0;
      r[i] = 0.0;
      stats[i] = Stats();
    }

    return std::make_unique<SARSALearner>(learner_id, n, g, b1, b2, weights,
                                          stats, m, r, learn_logger);
  }

  SARSALearner(int learner_id, double n, double g, double b1, double b2,
               double weights[N], Stats s[N],
               double m[N],  double r[N],
               Logger* learn_logger)
    : learner_id_(learner_id),
      n_(n),
      g_(g),
      b1_(b1),
      b2_(b2),
      learn_logger_(learn_logger) {
    for(size_t i=0; i<N; i++) {
      weights_[i] = weights[i];
      feature_stats_[i] = s[i];
      m_[i] = m[i];
      r_[i] = r[i];
    }
  }

  double Learn(CVC* cvc, ExperienceImpl<N>* experience) {
    Action* action = experience->action_.get();

    assert(action);
    double updated_score = Score(experience->features_);

    //SARSA-FA:
    //from https://artint.info/html/ArtInt_272.html
    //Q(s, a) = r + g * Q(s', a')
    //where Q(s, a) was estimated
    //we had some error on that which we can gradient descend
    //experience: (s, a, r, s', a')
    //  d = r + g*Q(s', a') - Q(s, a)
    //  for each w_i:
    //      w_i = w_i + n * d * F_i(s, a)
    //
    //where
    //r is the reward that was calculated during EvaluteQueuedActions
    //g is the discount factor (discounting future returns)
    //n is the learning rate (gradient descent step-size)
    //Q(s, a) is the score of a (previous estimate of future score)
    //Q(s', a') is the score of a' (current estimate of future score)
    //w are the weights that were used to choose a
    //F(s, a) are the features that go with a
    //
    //estimate of score (y_hat) is
    //y_hat = sum_i(w_i * x_i)
    //where x_0 = 1 (bias term)
    //
    //Loss function is
    //(y_hat - y)^2 (squared error)
    //d_(y_hat) = 2 * (y_hat - y)
    //d_(w_i) = x_i
    //
    //weight update is
    //w_i <- w_i - n *( d_L/d_(y_hat) * d_(y_hat)/d_(w_i) )

    //compute (estimate) the partial derivative w.r.t. score
    double truth_estimate = ComputeDiscountedRewards(experience);
    double loss = pow(updated_score - truth_estimate, 2);
    double dL_dy = 2 * (updated_score - truth_estimate);
    assert(!std::isinf(dL_dy));

    //log some info about model performance
    learn_logger_->Log(INFO, "%d\t%s\t%d\t%f\t%f\t%f\t%f\t%f\n", cvc->Now(),
                       action->GetActionId(), learner_id_, loss, dL_dy,
                       updated_score, truth_estimate,
                       experience->next_experience_->score_ -
                           experience->score_);

    //TODO: clean up these needless lines
    //this might not be the case if someone has changed the weights since we
    //assert(action->GetScore() == Score(action->GetFeatureVector()));
    // update the weights
    //assert(action->GetFeatureVector().size() == weights_.size());
    //double n = n_;// / (double)(action->GetFeatureVector().size());
    //hang on to the sum of the partials for debugging
    double sum_d = 0.0;
    t_ += 1;
    for(size_t i = 0; i < N; i++) {
      //keep some stats on the features for later analysis
      feature_stats_[i].Update(experience->features_[i]);

      //simple learning
      //double weight_update = n * dL_dy * action->GetFeatureVector()[i];

      //partial derivative of loss w.r.t. this weight
      //by chain rule dL_dy * dy_dw
      double dL_dw = dL_dy * experience->features_[i];

      // ADAM optimizier
      // as per https://arxiv.org/pdf/1412.6980.pdf
      // taken from slides:
      // https://moodle2.cs.huji.ac.il/nu15/pluginfile.php/316969/mod_resource/content/1/adam_pres.pdf
      m_[i] = b1_*m_[i] + (1.0-b1_)*(dL_dw);
      r_[i] = b2_*r_[i] + (1.0-b2_)*(dL_dw * dL_dw);
      double m_hat = m_[i] / (1.0 - pow(b1_, t_));
      double r_hat = r_[i] / (1.0 - pow(b2_, t_));
      double weight_update = n_ * m_hat / sqrt(r_hat + epsilon_);


      assert(!std::isinf(weight_update));
      assert(!std::isnan(weight_update));
      weights_[i] = weights_[i] - weight_update;
      sum_d += dL_dy * experience->features_[i];
    }

    double new_score = Score(experience->features_);
    learn_logger_->Log(DEBUG, "after update:\t%s\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
                       action->GetActionId(), new_score, updated_score,
                       truth_estimate, (new_score - updated_score), dL_dy,
                       (new_score - updated_score) / dL_dy, n_);

    // ideally we don't overshoot, but it's technically possible
    /*assert((updated_score - truth_estimate) * (updated_score - truth_estimate) >
           (new_score - truth_estimate) * (new_score - truth_estimate));*/

    // check that we are doing it right:
    // error should get smaller
    //assert((truth_estimate - Score(action->GetFeatureVector())) <= abs(truth_estimate - action->GetScore()));
    // according to Hendrickson, the difference between the old score and the new
    // one should be the error computed above times the learning rate
    //assert(action->GetScore() - Score(action->GetFeatureVector()) == dL_dy * n_);

    return dL_dy;
  }

  void WriteWeights(FILE* weights_file) {
    size_t num_weights = N;
    fwrite(&num_weights, sizeof(size_t), 1, weights_file);
    for(double w : weights_) {
      fwrite(&w, sizeof(double), 1, weights_file);
    }
  }

  void ReadWeights(FILE* weights_file) {
    size_t count;
    int ret = fread(&count, sizeof(size_t), 1, weights_file);
    assert(1 == ret);
    assert(count == N);
    for(size_t i=0; i<N; i++) {
      double w;
      fread(&w, sizeof(double), 1, weights_file);
      weights_[i] = w;
    }
  }

  double Score(const double features[N]) const {
    //first feature had beter be bias term
    double score = 0.0;
    for(size_t i = 0; i < N; i++) {
      score += weights_[i] * features[i];
    }
    assert(!std::isinf(score));
    assert(!std::isnan(score));
    return score;
  }

  double ComputeDiscountedRewards(const Experience* experience) const {
    const Experience *e = experience;
    assert(e->next_experience_);
    double discounted_rewards = 0.0;
    int i = 0;
    while(e->next_experience_) {
      // reward is diff between score after action plays out minus score at time
      // of choosing action (this can get complicated if there's a bunch of other
      // stuff going on at the same time)
      double reward = e->next_experience_->score_ - e->score_;
      //TODO: what does it mean if this->g_ != e->learner->g_ ?
      discounted_rewards += pow(g_, i) * reward;
      e = e->next_experience_;
      i++;
    }

    return discounted_rewards + pow(g_, i) * e->PredictScore();
  }

  std::unique_ptr<Experience> WrapAction(double features[N],
                                         std::unique_ptr<Action> action) {
    action->SetScore(Score(features));
    return std::make_unique<ExperienceImpl<N>>(std::move(action), 0.0, nullptr,
                                               features, this);
  }

 private:
  int learner_id_;

  double n_; //learning rate
  double g_; //discount factor
  double weights_[N];

  Stats feature_stats_[N];

  //adam optimizer params and state
  double b1_;
  double b2_;
  //TODO: parametrize ADAM epsilon?
  double epsilon_ = .000000001; //10^-8
  //TODO: setting learning epoch always to 0 means we can't load state
  //so this would need to get serialized out
  int t_=0;
  double m_[N];
  double r_[N];

  Logger* learn_logger_;
};

// just one kind of action, one model
template<size_t N>
class SARSAActionFactory : public ActionFactory {
 public:
  static SARSALearner<N> CreateLearner(int learner_id, double n, double g,
                                       double b1, double b2,
                                       std::mt19937* random_generator,
                                       Logger* learn_logger) {
    double weights[N];
    Stats stats[N];
    double m[N];
    double r[N];

    std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
    for (size_t i = 0; i < N; i++) {
      weights[i] = weight_dist(*random_generator);
      m[i] = 0.0;
      r[i] = 0.0;
      stats[i] = Stats();
    }
    return SARSALearner<N>(learner_id, n, g, b1, b2, weights, stats, m, r,
                           learn_logger);
  }

  SARSAActionFactory(SARSALearner<N> learner) : learner_(learner) {}

  virtual ~SARSAActionFactory() {}

  virtual double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;

 protected:
  SARSALearner<N> learner_;
};

// just one kind of response, one model
template<size_t N>
class SARSAResponseFactory : public ResponseFactory {
 public:

  static SARSALearner<N> CreateLearner(int learner_id, double n, double g,
                                       double b1, double b2,
                                       std::mt19937* random_generator,
                                       Logger* learn_logger) {
    double weights[N];
    Stats stats[N];
    double m[N];
    double r[N];

    std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
    for (size_t i = 0; i < N; i++) {
      weights[i] = weight_dist(*random_generator);
      m[i] = 0.0;
      r[i] = 0.0;
      stats[i] = Stats();
    }
    return SARSALearner<N>(learner_id, n, g, b1, b2, weights, stats, m, r,
                           learn_logger);
  }

  SARSAResponseFactory(SARSALearner<N> learner) : learner_(learner) {}
  virtual ~SARSAResponseFactory() {}

  virtual double Respond(
      CVC* cvc, Character* character, Action* action,
      std::vector<std::unique_ptr<Experience>>* actions) = 0;
 protected:
  SARSALearner<N> learner_;
};

} //namespace cvc::sarsa

#endif
