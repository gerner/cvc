#ifndef CRUNCHEDIN_H_
#define CRUNCHEDIN_H_

#include <limits>
#include <vector>

#include "../core.h"
#include "../action.h"

namespace cvc::crunchedin {

const size_t CULTURE_DIMENSIONS = 2;

class CurriculumVitae;
struct Role;

struct Organization {
  CurriculumVitae* ceo_;
  std::vector<Role*> current_staff_;
  int start_tick_;
  int end_tick_ = std::numeric_limits<int>::max();

  std::array<double, CULTURE_DIMENSIONS> culture_;

  //TODO: contributions is a placeholder for the total score for this org
  //    in the future this ought to be a more complicated function of products,
  //    customers, sales, etc.
  double contributions_ = 0.0;
};

struct Role {

  CurriculumVitae* cv_;
  Organization* org_;
  int start_tick_;
  int end_tick_ = std::numeric_limits<int>::max();

  //TODO: contribution is a placeholder for contributions by this character
  //    in the future this ought to be a more complicated function of stuff
  //    this character has done
  double contribution_ = 0.0;

  //represents how "good" the org was when this character had this role
  //together represents how much the org improved during the character's tenure
  //useful to understand the character's role in that improvement, e.g.
  //contribution_ / (contributions_at_end_ - contributions_at_start_)
  //the value of corp contributions at start/end from this role
  //end is meaningless unless end_tick_ <= CVC.Now()
  double contributions_at_start_;
  double contributions_at_end_;
};

//records career history, but also the crunchedin representation of a character
class CurriculumVitae {
 public:
  double TotalContribution() const {
    double score = 0.0;
    for(auto& role: roles_) {
      score += role->contribution_;
    }
    return score;
  }

  void Contribute(Role* role, double contribution) {
    assert(this == role->cv_);
    // contribution is scaled by cosine similarity (itself scaled to (0,1] )
    double scale = 0.0;
    for (size_t i = 0; i < CULTURE_DIMENSIONS; i++) {
      scale += culture_[i] * role->org_->culture_[i];
    }
    // we assume culture vectors are unit vectors
    assert(scale >= -1.0);
    assert(scale <= 1.0);

    //scale to (0,1]
    scale /= 0.5;
    scale += 0.5;

    contribution *= scale;

    role->contribution_ += contribution;
    role->org_->contributions_ += contribution;
  }

  std::array<double, CULTURE_DIMENSIONS> GetCulture() const {
    return culture_;
  }

  Role* GetCurrentRole() const {
    return roles_.back().get();
  }

 private:
  std::vector<std::unique_ptr<Role>> roles_;
  std::array<double, CULTURE_DIMENSIONS> culture_;
};

struct CrunchedIn {
  std::vector<std::unique_ptr<Organization>> orgs_;
  std::vector<std::unique_ptr<CurriculumVitae>> cvs_;
  std::unordered_map<Character*, CurriculumVitae*> cv_lookup_;
};

class WorkAction : public Action {
 public:
  WorkAction(Character* character, double score, Role* role,
             double contribution)
      : Action(__FUNCTION__, character, score),
        role_(role),
        contribution_(contribution) {}

  bool IsValid(const CVC* cvc) override {
    //make sure we're still employed
    return cvc->Now() > role_->end_tick_;
  }

  void TakeEffect(CVC* gamestate) override {
    role_->cv_->Contribute(role_, contribution_);
  }

 private:
  Role* role_;
  double contribution_;
};

class ContributionScorer {
 public:
  double Score(CVC* cvc, Character* character) {
    CurriculumVitae* cv = crunchedin_->cv_lookup_[character];
    assert(cv);
    return cv->TotalContribution();
  }

 private:
  CrunchedIn* crunchedin_;
};

/*class ApplyAction : public Action {
 public:
  ApplyAction(Character* actor, double score, Character* target, Role* role)
      : Action(__FUNCTION__, actor, target, score),

        bool IsValid(const CVC* cvc) override {
    return true;
  }

  void TakeEffect(CVC* gamestate) override {
    //...just requires a response...
  }

  bool RequiresResponse() { return true; }

 private:

};*/

} //namespace cvc::crunchedin

#endif
