#ifndef CRUNCHEDIN_ACTION_FACTORIES_H_
#define CRUNCHEDIN_ACTION_FACTORIES_H_

#include <vector>
#include <memory>
#include <array>

#include "crunchedin.h"
#include "../sarsa/sarsa_agent.h"
#include "../sarsa/sarsa_learner.h"

namespace cvc::crunchedin {

const size_t work_action_features = CULTURE_DIMENSIONS*2 + 1;
class WorkActionFactory
    : public sarsa::SARSAActionFactory<work_action_features> {
 public:
  WorkActionFactory(sarsa::SARSALearner<work_action_features> learner)
      : sarsa::SARSAActionFactory<work_action_features>(learner) {}

  double EnumerateActions(
      CVC* cvc, Character* character,
      std::vector<std::unique_ptr<sarsa::Experience>>* actions) override {
    std::array<double, work_action_features> features;

    CurriculumVitae* cv = crunchedin_->cv_lookup_[character];
    //the character better exist in crunchedin
    assert(cv);

    Role* role = cv->GetCurrentRole();
    if (!role) {
      return 0.0;
    }

    //first feature is always bias term
    features[0] = 1.0;

    //next features are product of character culture and org culture
    size_t i=0;
    for(size_t j=0; j<CULTURE_DIMENSIONS; j++, i++) {
      features[i] =
          cv->GetCulture()[j] * role->org_->culture_[j];
    }

    actions->push_back(learner_.WrapAction(
        features, std::make_unique<WorkAction>(character, 0.0, role, 10.0)));
    return actions->back()->action_->GetScore();
  }
 private:
  CrunchedIn* crunchedin_;
};

}
#endif //CRUNCHEDIN_ACTION_FACTORIES_H_
