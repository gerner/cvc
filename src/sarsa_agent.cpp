include <vector>
#include <cassert>
#include <random>
#include <limits>
#include <cmath>
#include <cstring>
#include <memory>

#include "core.h"
#include "action.h"
#include "sarsa_agent.h"

void SARSALearner::ReadWeights(
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

void SARSALearner::WriteWeights(
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

void SARSALearner::WriteWeights(FILE* weights_file) {
  size_t num_weights = weights_.size();
  fwrite(&num_weights, sizeof(size_t), 1, weights_file);
  for(double w : weights_) {
    fwrite(&w, sizeof(double), 1, weights_file);
  }
}

void SARSALearner::ReadWeights(FILE* weights_file) {
  size_t count;
  int ret = fread(&count, sizeof(size_t), 1, weights_file);
  assert(1 == ret);
  assert(count == weights_.size());
  weights_.clear();
  for(size_t i=0; i<count; i++) {
    double w;
    fread(&w, sizeof(double), 1, weights_file);
    weights_.push_back(w);
  }
}

std::unique_ptr<SARSALearner> SARSALearner::Create(
    double n, double g, double b1, double b2, std::mt19937& random_generator,
    size_t num_features, Logger* learn_logger) {
  std::vector<double> weights;
  std::vector<Stats> stats;

  std::vector<double> m;
  std::vector<double> r;

  std::uniform_real_distribution<> weight_dist(-1.0, 1.0);
  for (size_t i = 0; i < num_features; i++) {
    weights.push_back(weight_dist(random_generator));
    m.push_back(0.0);
    r.push_back(0.0);
    stats.push_back(Stats());
  }

  return std::make_unique<SARSALearner>(n, g, b1, b2, weights, stats, m, r,
                                        learn_logger);
}

SARSALearner::SARSALearner(double n, double g, double b1, double b2,
                           std::vector<double> weights, std::vector<Stats> s,
                           std::vector<double> m, std::vector<double> r,
                           Logger* learn_logger)
    : n_(n),
      g_(g),
      weights_(weights),
      feature_stats_(s),
      b1_(b1),
      b2_(b2),
      m_(m),
      r_(r),
      learn_logger_(learn_logger) {}

void SARSALearner::Learn(CVC* cvc, Experience* experience) {
  Action* action = experience->action_.get();

  assert(action);
  double updated_score = Score(action->GetFeatureVector());

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
  double d = 2 * (updated_score - truth_estimate);//action->GetScore();
  assert(!std::isinf(d));

  //log:
  // tick
  // action
  // reward
  // estimated score
  // r + g * next estimated score
  // d
  // |
  // features
  // |
  // weights
  learn_logger_->Log(INFO, "%d\t%s\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
                     cvc->Now(), action->GetActionId(),
                     experience->next_experience_->score_ - experience->score_,
                     updated_score, truth_estimate, d,
                     action->GetFeatureVector()[0],
                     action->GetFeatureVector()[1], weights_[0], weights_[1]);

  //this might not be the case if someone has changed the weights since we
  //assert(action->GetScore() == Score(action->GetFeatureVector()));
  // update the weights
  assert(action->GetFeatureVector().size() == weights_.size());
  //double n = n_;// / (double)(action->GetFeatureVector().size());
  //hang on to the sum of the partials for debugging
  double sum_d = 0.0;
  t_ += 1;
  for(size_t i = 0; i < action->GetFeatureVector().size(); i++) {
    //keep some stats on the features for later analysis
    feature_stats_[i].Update(action->GetFeatureVector()[i]);

    //simple learning
    //double weight_update = n * d * action->GetFeatureVector()[i];

    //partial derivative of loss w.r.t. this weight
    double dL_dw = d*action->GetFeatureVector()[i];

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
    sum_d += d * action->GetFeatureVector()[i];
  }

  double new_score = Score(action->GetFeatureVector());
  learn_logger_->Log(INFO, "after update:\t%s\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
                     action->GetActionId(), new_score, updated_score,
                     truth_estimate, (new_score - updated_score), d,
                     (new_score - updated_score) / d, n_);

  // ideally we don't overshoot, but it's technically possible
  /*assert((updated_score - truth_estimate) * (updated_score - truth_estimate) >
         (new_score - truth_estimate) * (new_score - truth_estimate));*/

  // check that we are doing it right:
  // error should get smaller
  //assert((truth_estimate - Score(action->GetFeatureVector())) <= abs(truth_estimate - action->GetScore()));
  // according to Hendrickson, the difference between the old score and the new
  // one should be the error computed above times the learning rate
  //assert(action->GetScore() - Score(action->GetFeatureVector()) == d * n_);
}

double SARSALearner::ComputeDiscountedRewards(
    const Experience* experience) const {
  const Experience* e = experience;
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
  assert(e->learner_);

  return discounted_rewards + pow(g_, i) * e->learner_->Score(e->action_->GetFeatureVector());
}

double SARSALearner::Score(const std::vector<double>& features) {
  //first feature had beter be bias term
  assert(features.size() == weights_.size());
  double score = 0.0;
  for(size_t i = 0; i < features.size(); i++) {
    score += weights_[i] * features[i];
  }
  assert(!std::isinf(score));
  assert(!std::isnan(score));
  return score;
}

SARSAActionFactory::SARSAActionFactory(std::unique_ptr<SARSALearner> learner)
    : learner_(std::move(learner)) {}

Action* SARSAAgent::ChooseAction(CVC* cvc) {
  //if we have what was previously the next action, stick it in the set of
  //experiences
  if(next_action_) {
    experience_queue_.front().push_back(std::move(next_action_));
  }

  // list the choices of actions
  std::vector<std::unique_ptr<Experience>> actions;

  double score = 0.0;
  for (SARSAActionFactory* factory : action_factories_) {
    score += factory->EnumerateActions(cvc, character_, &actions);
  }

  // choose one according to the policy and store it, along with this agent in
  // a partial Experience which we will fill out later
  next_action_ = policy_->ChooseAction(&actions, cvc, character_);

  //TODO: should really support other kinds of objectives than just money
  //keep track of the current score at the time this action was chosen
  next_action_->score_ = Score(cvc);

  return next_action_->action_.get();
}

Action* SARSAAgent::Respond(CVC* cvc, Action* action) {
  assert(action->GetActor() != character_);
  assert(action->GetTarget() == character_);

  //1. find the appropriate response factory
  assert(response_factories_.find(action->GetActionId()) !=
         response_factories_.end());

  //2. ask it to enumerate some (scored) responses
  std::vector<std::unique_ptr<Experience>> actions;

  double score = 0.0;
  for (SARSAResponseFactory* factory :
       response_factories_[action->GetActionId()]) {
    score += factory->Respond(cvc, character_, action, &actions);
  }

  //3. choose
  experience_queue_.front().push_back(
      policy_->ChooseAction(&actions, cvc, character_));

  experience_queue_.front().back()->score_ = Score(cvc);
  return experience_queue_.front().back()->action_.get();
}

void SARSAAgent::Learn(CVC* cvc) {
  // we've just wrapped up a turn, so now is a good time to incorporate
  // information about the reward we received this turn.
  // 1. update rewards for all experiences (the learner does this when it
  // learns)

  //set the next experience pointer on the latest experiences
  for(auto& experience : experience_queue_.front()) {
    experience->next_experience_ = next_action_.get();
  }

  // 2. learn if necessary
  if(experience_queue_.size() == n_steps_) {
    for(auto& experience : experience_queue_.back()) {
      experience->learner_->Learn(cvc, experience.get());
    }
    //toss the experiences from which we just learned (at the back)
    experience_queue_.pop_back();
  }

  //set up space for the next batch of experiences (for the upcoming turn)
  experience_queue_.push_front(std::vector<std::unique_ptr<Experience>>());
}

double SARSAAgent::Score(CVC* cvc) {
  //TODO: maybe abstract this?
  //agent wants to make (and have) lots of money
  return character_->GetMoney();

  //return -abs(10000 - character_->GetMoney());

  //agent wants to have more money than average
  //return character_->GetMoney() - cvc->GetMoneyStats().max_;

  //return character_->GetMoney() / cvc->GetMoneyStats().mean_;

  //agent wants everyone to like them at least a little
  //return cvc->GetOpinionOfStats(GetCharacter()->GetId()).min_;

  //agent wants to be liked more than average
  //return cvc->GetOpinionOfStats(GetCharacter()->GetId()).mean_ - cvc->GetOpinionStats().mean_;
}

