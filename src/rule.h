#pragma once

#include "re.h"
#include <nlohmann/json.hpp>

namespace kaputt
{
using json = nlohmann::json;

struct RuleBase
{
    virtual json getDefaultParams()              = 0;
    virtual bool checkParams(const json& params) = 0;
    virtual void drawParams(json& params)        = 0;

    virtual bool check(const json& params, const RE::Actor* attacker, const RE::Actor* victim) = 0;

    virtual constexpr std::string_view getName() = 0;
    virtual constexpr std::string_view getHint() = 0;
};

struct RuleParamsBase
{
    virtual void draw() = 0;
};

template <class ParamsClass>
struct Rule : RuleBase
{
    static_assert(std::is_base_of_v<RuleParamsBase, ParamsClass>);

    virtual inline json getDefaultParams() { return ParamsClass(); }
    virtual inline bool checkParams(const json& params)
    {
        try
        {
            ParamsClass obj = params;
        }
        catch (json::exception)
        {
            return false;
        }
        return true;
    }
    virtual inline void drawParams(json& params)
    {
        ParamsClass obj = params;
        obj.draw();
        params = obj;
    };

    virtual bool        check(const ParamsClass& params, const RE::Actor* attacker, const RE::Actor* victim) = 0;
    virtual inline bool check(const json& params, const RE::Actor* attacker, const RE::Actor* victim)
    {
        ParamsClass obj = params;
        return check(obj, attacker, victim);
    }
};

struct SingleActorRuleParams : RuleParamsBase
{
    bool         check_attacker = false;
    virtual void draw();
};

struct DummyRuleParams : RuleParamsBase
{
    bool                dummy = true;
    virtual inline void draw() {}
};

//////////////////////////////////////////////////////////////////////// ACTUAL STUFF

#define RULE_NAME_HINT(name, hint)                                \
    virtual constexpr std::string_view getName() { return name; } \
    virtual constexpr std::string_view getHint() { return hint; }

struct UnconditionalRuleParams : RuleParamsBase
{
    bool         value = true;
    virtual void draw();
};
struct UnconditionalRule : Rule<UnconditionalRuleParams>
{
    RULE_NAME_HINT("Unconditional", "Always True.")
    virtual inline bool check(const UnconditionalRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim) { return params.value; }
};

struct PlayableRule : Rule<SingleActorRuleParams>
{
    RULE_NAME_HINT("Animation Playable", "True if actor can play paired animations.\n"
                                         "i.e. loaded, alive, not already playing animation, and not mounted.")
    virtual inline bool check(const SingleActorRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim)
    {
        auto actor = params.check_attacker ? attacker : victim;
        return actor->Is3DLoaded() && !actor->IsDead() && !isInPairedAnimation(actor) && !actor->IsOnMount();
    }
};

struct BleedoutRule : Rule<SingleActorRuleParams>
{
    RULE_NAME_HINT("Bleedout", "True if actor is bleeding out.")
    virtual inline bool check(const SingleActorRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim)
    {
        return isBleedout(params.check_attacker ? attacker : victim);
    }
};

struct RagdollRule : Rule<SingleActorRuleParams>
{
    RULE_NAME_HINT("Ragdoll", "True if actor is ragdolling.")
    virtual inline bool check(const SingleActorRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim)
    {
        return (params.check_attacker ? attacker : victim)->IsInRagdollState(); // Alternate method: check knockState
    }
};

struct ProtectedRule : Rule<DummyRuleParams>
{
    RULE_NAME_HINT("Protected", "True if victim is protected and attacker is not player.")
    virtual inline bool check(const DummyRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim)
    {
        return !attacker->IsPlayerRef() && victim->GetActorRuntimeData().boolFlags.all(RE::Actor::BOOL_FLAGS::kProtected);
    }
};

struct EssentialRule : Rule<SingleActorRuleParams>
{
    RULE_NAME_HINT("Essential", "True if actor is essential.")
    virtual inline bool check(const SingleActorRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim)
    {
        return (params.check_attacker ? attacker : victim)->GetActorRuntimeData().boolFlags.all(RE::Actor::BOOL_FLAGS::kEssential);
    }
};

struct AngleRuleParams : RuleParamsBase
{
    float        angle_min = -45.f;
    float        angle_max = 45.f;
    virtual void draw();
};
struct AngleRule : Rule<AngleRuleParams>
{
    RULE_NAME_HINT("Relative Angle", "True if the attacker is between 2 angles relative to the victim's facing.\n Ranges from -360 to 360 deg clockwise, 0 being straight ahead.")
    virtual bool check(const AngleRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim);
};

struct LastHostileInRangeRuleParams : RuleParamsBase
{
    float        range = 1024;
    virtual void draw();
};
struct LastHostileInRangeRule : Rule<LastHostileInRangeRuleParams>
{
    RULE_NAME_HINT("Last Hostile", "True if the victim is the last hostile actor within certain distance (1024 ~= 15 m/48').")
    virtual bool check(const LastHostileInRangeRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim);
};

struct SkeletonRuleParams : SingleActorRuleParams
{
    std::string  skeleton = "";
    virtual void draw();
};
struct SkeletonRule : Rule<SkeletonRuleParams>
{
    RULE_NAME_HINT("Skeleton", "True if the actor's skeleton matches. For race checks.")
    virtual bool check(const SkeletonRuleParams& params, const RE::Actor* attacker, const RE::Actor* victim);
};

//////////////////////////////////////////////////////////////////////// CEREALIZATION

const StrMap<std::shared_ptr<RuleBase>>& getRule();

// This is for better fitting json serialization to avoid using pointers in class
struct RuleInfo
{
    bool enabled   = true; // for other purposes
    bool need_true = true;

    std::string type    = "";
    std::string comment = "";
    json        params  = {};

    RuleInfo() = default;
    inline RuleInfo(std::string_view type) :
        type(type)
    {
        auto p_rule = getRule().find(type);
        if (p_rule == getRule().end())
            enabled = false;
        else
            params = p_rule->second->getDefaultParams();
    }

    // THESE WILL THROW
    inline bool             check(const RE::Actor* attacker, const RE::Actor* victim) const { return getRule().at(type)->check(params, attacker, victim); }
    inline std::string_view getHint() { return getRule().at(type)->getHint(); }
    inline void             drawParams() { getRule().at(type)->drawParams(params); }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RuleInfo, type, enabled, need_true, params, comment)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DummyRuleParams, dummy)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SingleActorRuleParams, check_attacker)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UnconditionalRuleParams, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AngleRuleParams, angle_min, angle_max)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LastHostileInRangeRuleParams, range)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SkeletonRuleParams, check_attacker, skeleton)

} // namespace kaputt
