#include <algorithm>
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

double SARSATrivialActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Experience>>* actions) {
  double features[6];
  StandardFeatures(cvc, character, features);
  actions->push_back(learner_->WrapAction(features,
      std::make_unique<TrivialAction>(character, 0.0)));
  return actions->back()->action_->GetScore();
}

double SARSAGiveActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Experience>>* actions) {
  if (character->GetMoney() > 10.0) {
    //choose a single target to potentially give to
    double sum_score = 0.0;
    for (Character* target : cvc->GetCharacters()) {
      if(target == character) {
        continue;
      }
      double features[10];

      //add this as an action choice
      actions->push_back(
          learner_->WrapAction(TargetFeatures(cvc, character, target, features),
                               std::make_unique<GiveAction>(
                                   character, 0.0, target, 10.0)));
      sum_score += actions->back()->action_->GetScore();

    }
    return sum_score;
  }
  return 0.0;
}

double SARSAAskActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Experience>>* actions) {

  double sum_score = 0.0;
  for (Character* target : cvc->GetCharacters()) {
    // skip self
    if (character == target) {
      continue;
    }

    if (target->GetMoney() <= 10.0) {
      continue;
    }
    double features[10];
    //add this as an option to ask
    actions->push_back(learner_->WrapAction(
        TargetFeatures(cvc, character, target, features),
        std::make_unique<AskAction>(character, 0.0, target, 10.0)));
    sum_score += actions->back()->action_->GetScore();
  }
  return sum_score;
}

double SARSAAskSuccessResponseFactory::Respond(
    CVC* cvc, Character* character, Action* action,
    std::vector<std::unique_ptr<Experience>>* actions) {
  //action->GetTarget() is asking us for action->GetRequestAmount() money
  AskAction* ask_action = (AskAction*)action;

  if(character->GetMoney() < ask_action->GetRequestAmount()) {
    return 0.0;
  }

  double features[10];
  actions->push_back(learner_->WrapAction(
      TargetFeatures(cvc, character, ask_action->GetTarget(), features),
      std::make_unique<AskSuccessAction>(character, 0.0, ask_action->GetActor(),
                                         ask_action)));
  return actions->back()->action_->GetScore();
}

double SARSAAskFailureResponseFactory::Respond(
    CVC* cvc, Character* character, Action* action,
    std::vector<std::unique_ptr<Experience>>* actions) {
  //action->GetTarget() is asking us for action->GetRequestAmount() money
  AskAction* ask_action = (AskAction*)action;

  double features[10];
  actions->push_back(learner_->WrapAction(
      TargetFeatures(cvc, character, ask_action->GetTarget(), features),
      std::make_unique<TrivialResponse>(character, 0.0)));
  return actions->back()->action_->GetScore();
}

double SARSAWorkActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Experience>>* actions) {
  double features[10];
  actions->push_back(
      learner_->WrapAction(StandardFeatures(cvc, character, features),
                           std::make_unique<WorkAction>(character, 0.0)));
  return actions->back()->action_->GetScore();
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

std::unique_ptr<Experience> SoftmaxPolicy::ChooseAction(
    std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
    Character* character) {
  assert(actions->size() > 0);

  double scores[actions->size()];
  double sum_score = 0.0;

  //sort by score so index corresponds to ordering
  //however, this is quite slow if there are a lot of choices
  //this is useful for logging the position of the option chosen for analysis
  /*std::sort((*actions).begin(), (*actions).end(),
            [](std::unique_ptr<Experience>& a, std::unique_ptr<Experience>& b) {
              return a->action_->GetScore() > b->action_->GetScore();
            });*/

  for (size_t i = 0; i < actions->size(); i++) {
    scores[i] = exp((*actions)[i]->action_->GetScore() / temperature_);
    assert(!std::isinf(scores[i]));
    assert(!std::isnan(scores[i]));
    sum_score += scores[i];
  }

  // TODO: this is very sensitive to have the best option overwhelmed by
  // numerous "similar" actions (e.g. a single work action compared to an ask
  // action for every other character)

  // choose one according to softmax
  std::uniform_real_distribution<> dist(0.0, 1.0);
  double choice = dist(*cvc->GetRandomGenerator());
  double sum_prob = 0.0;

  for (size_t i = 0; i < actions->size(); i++) {
    sum_prob += scores[i]/sum_score;
    if (choice < sum_prob) {
      logger_->Log(INFO,
                   "%d chose %s with score %f with prob %f (choice %f temp %f) at "
                   "position %zu of %zu\n", cvc->Now(),
                   (*actions)[i]->action_->GetActionId(),
                   (*actions)[i]->action_->GetScore(), scores[i] / sum_score,
                   choice, temperature_, i, actions->size());
      assert((*actions)[i]->action_->IsValid(cvc));
      return std::move((*actions)[i]);
    }
  }
  abort();
}

std::unique_ptr<Experience> AnnealingSoftmaxPolicy::ChooseAction(
    std::vector<std::unique_ptr<Experience>>* actions, CVC* cvc,
    Character* character) {
  temperature_ = initial_temperature_ / sqrt(cvc->Now()+1);
  return SoftmaxPolicy::ChooseAction(actions, cvc, character);
}

void GradSensitiveSoftmaxPolicy::UpdateGrad(double dL_dy, double y) {
  // TODO: a few issues with this:
  // the min/max thing is really clunky
  // poor foundation to divide by y and square I think
  // this will raise the temp if we start getting bigger gradients for actions
  // chosen, but not if actions we didn't choose start changing (since we won't
  // observe that)
  temperature_ = decay_ * (scale_ * (dL_dy / y) * (dL_dy / y)) +
                 (1.0 - decay_) * temperature_;
  if(temperature_ < min_temperature_) {
    temperature_ = min_temperature_;
  }
  if(temperature_ > max_temperature_) {
    temperature_ = max_temperature_;
  }
}


