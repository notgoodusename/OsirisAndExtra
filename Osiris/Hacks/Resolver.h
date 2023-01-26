#pragma once

#include "Animations.h"
#include "../SDK/GameEvent.h"
#include "../SDK/Entity.h"

namespace resolver
{
    void reset() noexcept;

    void process_missed_shots() noexcept;
    void save_record(int player_index, float player_simulation_time) noexcept;
    void get_event(GameEvent* event) noexcept;

    void run_pre_update(Animations::Players& player, Entity* entity) noexcept;
    void run_post_update(Animations::Players& player, Entity* entity) noexcept;

    float resolve_shot(const Animations::Players& player, Entity* entity);

    void detect_side(Entity* entity, int* side);

    void setup_detect(Animations::Players& player, Entity* entity);

    void anti_one_tap(int userid, Entity* entity, Vector shot);

    void cmd_grabber(UserCmd* cmd);

    void resolve_entity(const Animations::Players& player, Entity* entity);

    void update_event_listeners(bool force_remove = false) noexcept;

    struct snap_shot
    {
        Animations::Players player;
        const Model* model{ };
        Vector eye_position{};
        Vector bullet_impact{};
        bool got_impact{ false };
        float time{ -1 };
        int player_index{ -1 };
        int backtrack_record{ -1 };
    };
}