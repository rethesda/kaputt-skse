#include "re.h"

#include "menu.h"

namespace kaputt
{
void ProcessHitHook::thunk(RE::Actor* a_victim, RE::HitData& a_hitData)
{
    // if (kaputt::PostHitTrigger::getSingleton()->process(a_victim, a_hitData))
    //     return func(a_victim, a_hitData);
}

RE::Actor* getNearestNPC(RE::Actor* origin, float max_range)
{
    logger::debug("getNearestNPC");
    auto process_lists = RE::ProcessLists::GetSingleton();
    if (!process_lists)
    {
        logger::error("Failed to get ProcessLists!");
        return nullptr;
    }
    auto n_load_actors = process_lists->numberHighActors;
    logger::debug("Number of high actors: {}.", n_load_actors);
    if (n_load_actors == 0)
        return nullptr;

    float      min_dist  = max_range;
    RE::Actor* min_actor = nullptr;
    for (auto actor_handle : process_lists->highActorHandles)
    {
        if (!actor_handle || !actor_handle.get())
            continue;

        auto actor = actor_handle.get().get();
        logger::debug("Checking actor: {}", actor->GetName());

        if (actor == origin)
            continue;

        float dist = actor->GetPosition().GetDistance(origin->GetPosition());
        if (dist < min_dist)
        {
            min_dist  = dist;
            min_actor = actor;
        }
    }

    if (!min_actor)
        logger::debug("No actor in range.");
    return min_actor;
}

void playPairedIdle(RE::TESIdleForm* idle, RE::Actor* attacker, RE::Actor* victim)
{
    auto edid = idle->GetFormEditorID();
    logger::debug("Now playing {} between {} and {}", edid, attacker->GetName(), victim->GetName());
    _playPairedIdle(attacker->GetActorRuntimeData().currentProcess, attacker, RE::DEFAULT_OBJECT::kActionIdle, idle, true, false, victim);
    kaputt::setStatusMessage(fmt::format("Last played by this mod: {}", edid)); // notify menu
}
void testPlayPairedIdle(RE::TESIdleForm* idle, float max_range)
{
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player)
    {
        logger::info("No player found!");
        return;
    }
    auto victim = getNearestNPC(player, max_range);
    if (!victim)
    {
        logger::info("No target found!");
        return;
    }

    playPairedIdle(idle, player, victim);
}

} // namespace kaputt