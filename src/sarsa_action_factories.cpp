#include <vector>
#include <cassert>
#include <random>
#include <limits>
#include <cmath>
#include <cstring>
#include <memory>

#include "core.h"
#include "action.h"
#include "sarsa_action_factories.h"

std::unique_ptr<SARSATrivialActionFactory> SARSATrivialActionFactory::Create(
    std::unique_ptr<SARSALearner> learner) {
  return std::make_unique<SARSATrivialActionFactory>(std::move(learner));
}

SARSATrivialActionFactory::SARSATrivialActionFactory(
    std::unique_ptr<SARSALearner> learner)
    : SARSAActionFactory(std::move(learner)) {}

double SARSATrivialActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Experience>>* actions) {
  /*std::vector<double> features(
      {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
       cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
       character->GetMoney(), cvc->GetOpinionByStats(character->GetId()).mean_,
       cvc->GetOpinionByStats(character->GetId()).stdev_,
       cvc->GetOpinionOfStats(character->GetId()).mean_,
       cvc->GetOpinionOfStats(character->GetId()).stdev_});*/
  //std::vector<double> features({1.0, log(character->GetMoney())});
  std::vector<double> features = Features(cvc, character);
  double score = learner_->Score(features);
  actions->push_back(learner_->WrapAction(
      std::make_unique<TrivialAction>(character, score, features)));
  return score;
}

std::unique_ptr<SARSAGiveActionFactory> SARSAGiveActionFactory::Create(
    std::unique_ptr<SARSALearner> learner) {
  return std::make_unique<SARSAGiveActionFactory>(std::move(learner));
}

SARSAGiveActionFactory::SARSAGiveActionFactory(
    std::unique_ptr<SARSALearner> learner)
    : SARSAActionFactory(std::move(learner)) {}

double SARSAGiveActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Experience>>* actions) {
  if (character->GetMoney() > 10.0) {
    //choose a single target to potentially give to
    double best_score = std::numeric_limits<double>::lowest();
    Character* best_target = NULL;
    std::vector<double> best_features;
    for (Character* target : cvc->GetCharacters()) {
      if(target == character) {
        continue;
      }
      //TODO: consider some other features
      /*std::vector<double> features(
          {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
           cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
           character->GetMoney(),
           cvc->GetOpinionByStats(character->GetId()).mean_,
           cvc->GetOpinionByStats(character->GetId()).stdev_,
           cvc->GetOpinionOfStats(character->GetId()).mean_,
           cvc->GetOpinionOfStats(character->GetId()).stdev_,
           character->GetOpinionOf(target), target->GetOpinionOf(character),
           target->GetMoney()});*/
      std::vector<double> features = TargetFeatures(cvc, character, target);
      double score = learner_->Score(features);
      if(score > best_score) {
        best_score = score;
        best_features = features;
        best_target = target;
      }
    }

    if (best_target) {
      actions->push_back(learner_->WrapAction(std::make_unique<GiveAction>(
          character, best_score, best_features, best_target, 10.0)));
      return best_score;
    }
  }
  return 0.0;
}

std::unique_ptr<SARSAAskActionFactory> SARSAAskActionFactory::Create(
    std::unique_ptr<SARSALearner> learner) {
  return std::make_unique<SARSAAskActionFactory>(std::move(learner));
}

SARSAAskActionFactory::SARSAAskActionFactory(std::unique_ptr<SARSALearner> learner)
    : SARSAActionFactory(std::move(learner)) {}

double SARSAAskActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Experience>>* actions) {

  Character* best_target = NULL;
  double best_score = std::numeric_limits<double>::lowest();
  std::vector<double> best_features;
  for (Character* target : cvc->GetCharacters()) {
    // skip self
    if (character == target) {
      continue;
    }

    if (target->GetMoney() <= 10.0) {
      continue;
    }
    //TODO: consider some other features
    /*std::vector<double> features(
        {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
         cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
         character->GetMoney(),
         cvc->GetOpinionByStats(character->GetId()).mean_,
         cvc->GetOpinionByStats(character->GetId()).stdev_,
         cvc->GetOpinionOfStats(character->GetId()).mean_,
         cvc->GetOpinionOfStats(character->GetId()).stdev_,
         character->GetOpinionOf(target), target->GetOpinionOf(character),
         target->GetMoney()});*/
    std::vector<double> features = TargetFeatures(cvc, character, target);
    double score = learner_->Score(features);
    if(score > best_score) {
      best_score = score;
      best_features = features;
      best_target = target;
    }
  }
  if (best_target) {
    actions->push_back(learner_->WrapAction(std::make_unique<AskAction>(
        character, best_score, best_features, best_target, 10.0)));
    return best_score;
  }
  return 0.0;
}

std::unique_ptr<SARSAAskSuccessResponseFactory>
SARSAAskSuccessResponseFactory::Create(std::unique_ptr<SARSALearner> learner) {
  return std::make_unique<SARSAAskSuccessResponseFactory>(std::move(learner));
}

SARSAAskSuccessResponseFactory::SARSAAskSuccessResponseFactory(
    std::unique_ptr<SARSALearner> learner)
    : SARSAResponseFactory(std::move(learner)) {}

double SARSAAskSuccessResponseFactory::Respond(
    CVC* cvc, Character* character, Action* action,
    std::vector<std::unique_ptr<Experience>>* actions) {
  //action->GetTarget() is asking us for action->GetRequestAmount() money
  AskAction* ask_action = (AskAction*)action;

  if(character->GetMoney() < ask_action->GetRequestAmount()) {
    return 0.0;
  }

  std::vector<double> features =
      TargetFeatures(cvc, character, ask_action->GetTarget());
  double score = learner_->Score(features);
  actions->push_back(learner_->WrapAction(std::make_unique<AskSuccessAction>(
      character, score, features, ask_action->GetActor(), ask_action)));
  return score;
}

std::unique_ptr<SARSAAskFailureResponseFactory>
SARSAAskFailureResponseFactory::Create(std::unique_ptr<SARSALearner> learner) {
  return std::make_unique<SARSAAskFailureResponseFactory>(std::move(learner));
}

SARSAAskFailureResponseFactory::SARSAAskFailureResponseFactory(
    std::unique_ptr<SARSALearner> learner)
    : SARSAResponseFactory(std::move(learner)) {}

double SARSAAskFailureResponseFactory::Respond(
    CVC* cvc, Character* character, Action* action,
    std::vector<std::unique_ptr<Experience>>* actions) {
  //action->GetTarget() is asking us for action->GetRequestAmount() money
  AskAction* ask_action = (AskAction*)action;

  std::vector<double> features =
      TargetFeatures(cvc, character, ask_action->GetTarget());
  double score = learner_->Score(features);
  actions->push_back(learner_->WrapAction(std::make_unique<TrivialResponse>(
      character, score, features)));
  return score;
}

std::unique_ptr<SARSAWorkActionFactory> SARSAWorkActionFactory::Create(
    std::unique_ptr<SARSALearner> learner) {
  return std::make_unique<SARSAWorkActionFactory>(std::move(learner));
}

SARSAWorkActionFactory::SARSAWorkActionFactory(std::unique_ptr<SARSALearner> learner)
    : SARSAActionFactory(std::move(learner)) {}

double SARSAWorkActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Experience>>* actions) {
  /*std::vector<double> features(
      {1.0, cvc->GetOpinionStats().mean_, cvc->GetOpinionStats().stdev_,
       cvc->GetMoneyStats().mean_, cvc->GetMoneyStats().stdev_,
       character->GetMoney(), cvc->GetOpinionByStats(character->GetId()).mean_,
       cvc->GetOpinionByStats(character->GetId()).stdev_,
       cvc->GetOpinionOfStats(character->GetId()).mean_,
       cvc->GetOpinionOfStats(character->GetId()).stdev_});*/
  std::vector<double> features = Features(cvc, character);
  double score = learner_->Score(features);
  actions->push_back(learner_->WrapAction(
      std::make_unique<WorkAction>(character, score, features)));
  return score;
}

std::unique_ptr<Experience> EpsilonGreedyPolicy::ChooseAction(
    std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
    Character* character) {
  // choose best action with prob 1-epsilon and a uniform random action with
  // prob epsilon

  assert(actions->size() > 0);

  //best or random?
  std::uniform_real_distribution<> dist(0.0, 1.0);
  double e = dist(*cvc->GetRandomGenerator());
  double best_score = std::numeric_limits<double>::lowest();
  std::unique_ptr<Experience>* best_action = nullptr;
  if(dist(*cvc->GetRandomGenerator()) > epsilon_) {
    logger_->Log(INFO, "choosing best (%f > %f)\n", e, epsilon_);
    //best choice
    for(std::unique_ptr<Experience>& experience : *actions) {
      logger_->Log(INFO, "option %s with score %f\n",
                   experience->action_->GetActionId(),
                   experience->action_->GetScore());
      if(experience->action_->GetScore() > best_score) {
        best_score = experience->action_->GetScore();
        best_action = &experience;
      }
    }
  } else {
    logger_->Log(INFO, "choosing random (%f >= %f)\n", e, epsilon_);
    //random choice
    int choice = dist(*cvc->GetRandomGenerator()) * actions->size();
    best_score = (*actions)[choice]->action_->GetScore();
    best_action = &(*actions)[choice];
  }
  assert(best_action);
  logger_->Log(INFO, "chose %s with score %f\n",
               (*best_action)->action_->GetActionId(), best_score);
  return std::move(*best_action);
}

