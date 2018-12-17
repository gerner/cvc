#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>

#include "core.h"
#include "action_factories.h"

double GiveActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  double score = 0.0;

  Character* best_target = NULL;
  double worst_opinion = std::numeric_limits<double>::max();
  if (character->GetMoney() > 10.0) {
    for (Character* target : cvc->GetCharacters()) {
      // skip self
      if (character == target) {
        continue;
      }

      // skip if target has above average money
      if (target->GetMoney() > cvc->GetMoneyStats().mean_) {
        continue;
      }

      // pick the character that likes us the least
      double opinion = target->GetOpinionOf(character);
      if (opinion < worst_opinion) {
        worst_opinion = opinion;
        best_target = target;
      }
    }
    if (best_target) {
      actions->push_back(std::make_unique<GiveAction>(
          character, 0.4, std::vector<double>({1.0, 0.2}), best_target,
          10.0));
      score = 0.4;
    }
  }
  return score;
}

double AskActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  double score = 0.0;

  Character* best_target = NULL;
  double best_opinion = 0.0;
  for (Character* target : cvc->GetCharacters()) {
    // skip self
    if (character == target) {
      continue;
    }

    if (target->GetMoney() <= 10.0) {
      continue;
    }

    // skip if target has below average money
    if (target->GetMoney() > cvc->GetMoneyStats().mean_) {
      continue;
    }

    // pick the character that likes us the least
    double opinion = target->GetOpinionOf(character);
    if (opinion > best_opinion) {
      best_opinion = opinion;
      best_target = target;
    }
  }
  if (best_target) {
    actions->push_back(std::make_unique<AskAction>(
        character, 0.4, std::vector<double>({0.7, 0.5}), best_target, 10.0));
    score = 0.4;
  }
  return score;
}

double AskResponseFactory::Respond(
    CVC* cvc, Character* character, Action* action,
    std::vector<std::unique_ptr<Action>>* responses) {

  AskAction* ask_action = (AskAction*)action;

  // check to see if the target will accept
  // opinion < 0 => no, otherwise some distribution improves with opinion
  double opinion = ask_action->GetTarget()->GetOpinionOf(ask_action->GetActor());
  std::uniform_real_distribution<> dist(0.0, 1.0);
  bool success = false;
  if (opinion > 0.0 && dist(*cvc->GetRandomGenerator()) <
                           1.0 / (1.0 + exp(-10.0 * (opinion - 0.5)))) {
    success = true;
  }

  if (success) {
    responses->push_back(std::make_unique<AskSuccessAction>(
        ask_action->GetTarget(), 1.0, std::vector<double>({1.0}),
        ask_action->GetActor(), ask_action));
  } else {
    // on failure:
    responses->push_back(std::make_unique<TrivialResponse>(
        ask_action->GetTarget(), 1.0, std::vector<double>({1.0})));
    // decrease opinion of actor (refused request)
    /*this->GetActor()->AddRelationship(std::make_unique<RelationshipModifier>(
        this->GetTarget(), gamestate->Now(), gamestate->Now() + 10,
        this->request_amount_));
    SetReward(0.0);
    gamestate->GetLogger()->Log(DEBUG, "request_failed by %d to %d of %f\n", this->GetActor()->GetId(),
           this->GetTarget()->GetId(), this->request_amount_);*/
  }
  return 1.0;
}

double WorkActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  for (Character* target : cvc->GetCharacters()) {
    if(target->GetOpinionOf(character) > 0.0) {
      actions->push_back(
          std::make_unique<WorkAction>(character, 0.3, std::vector<double>()));
      return 0.3;
    }
  }
  return 0.0;
}

double TrivialActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  actions->push_back(
      std::make_unique<TrivialAction>(character, 0.2, std::vector<double>()));
  return 0.2;
}

CompositeActionFactory::CompositeActionFactory(
    std::unordered_map<std::string, ActionFactory*> factories)
    : factories_(factories) {}

double CompositeActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  double score = 0.0;
  for(const auto& factory : factories_) {
    score += factory.second->EnumerateActions(cvc, character, actions);
  }
  return score;
}

std::unique_ptr<Action> ProbDistPolicy::ChooseAction(
    std::vector<std::unique_ptr<Action>>* actions, CVC* cvc,
    Character* character) {
  // there must be at least one action to choose from (even if it's trivial)
  assert(!actions->empty());

  double sum_score = 0.0;
  for(auto& action : *actions) {
    sum_score += action->GetScore();
  }

  // choose one
  std::uniform_real_distribution<> dist(0.0, 1.0);
  double choice = dist(*cvc->GetRandomGenerator());
  double sum_prob = 0;

  // DecisionEngine is responsible for maintaining the lifecycle of the action
  // so we'll move it to our state and ditch the rest when they go out of
  // scope
  for (std::unique_ptr<Action>& action : *actions) {
    sum_prob += action->GetScore() / sum_score;
    if (choice < sum_prob) {
      assert(action->IsValid(cvc));
      // keep this one action
      return std::move(action);
    }
  }
  assert(false);
}

