#include <vector>
#include <memory>

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
          character->GetMoney() * 0.1));
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
    std::vector<ActionFactory*> factories)
    : factories_(factories) {}

double CompositeActionFactory::EnumerateActions(
    CVC* cvc, Character* character,
    std::vector<std::unique_ptr<Action>>* actions) {
  double score = 0.0;
  for(ActionFactory* factory : factories_) {
    score += factory->EnumerateActions(cvc, character, actions);
  }
  return score;
}
