#include "namespaces.h"
#include "../opcode_arg_types/variables/all_core.h"
#include "../../../helpers/qt/string.h"

namespace Megalo {
   namespace Script {
      const NamespaceMember* Namespace::get_member(const QString& name) const noexcept {
         for (auto& member : this->members)
            if (cobb::qt::stricmp(name, member.name) == 0)
               return &member;
         return nullptr;
      }

      namespace namespaces {
         std::array<Namespace*, 3> list = { &unnamed, &global, &game };
         Namespace* get_by_name(const QString& name) {
            for (auto& ns : namespaces::list)
               if (cobb::qt::stricmp(name, ns->name) == 0)
                  return ns;
            return nullptr;
         }
         //
         Namespace unnamed = Namespace("game", true, false, {
            NamespaceMember::make_which_member("no_object",         OpcodeArgValueObject::typeinfo, 'none', NamespaceMember::flag::is_none),
            NamespaceMember::make_which_member("no_player",         OpcodeArgValuePlayer::typeinfo, 'none', NamespaceMember::flag::is_none),
            NamespaceMember::make_which_member("no_team",           OpcodeArgValueTeam::typeinfo,   'none', NamespaceMember::flag::is_none),
            NamespaceMember::make_which_member("current_object",    OpcodeArgValueObject::typeinfo, 'loop', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("current_player",    OpcodeArgValuePlayer::typeinfo, 'loop', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("current_team",      OpcodeArgValueTeam::typeinfo,   'loop', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("neutral_team",      OpcodeArgValueTeam::typeinfo,   'neut', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("hud_target_object", OpcodeArgValueObject::typeinfo, 'hudt', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("hud_target_player", OpcodeArgValuePlayer::typeinfo, 'hudt', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("hud_player",        OpcodeArgValuePlayer::typeinfo, 'hudp', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("killed_object",     OpcodeArgValueObject::typeinfo, 'kild', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("killer_object",     OpcodeArgValueObject::typeinfo, 'kilr', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("killer_player",     OpcodeArgValuePlayer::typeinfo, 'kilr', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("unk_14_team",       OpcodeArgValueTeam::typeinfo,   'u_14', NamespaceMember::flag::is_read_only),
            NamespaceMember::make_which_member("unk_15_team",       OpcodeArgValueTeam::typeinfo,   'u_14', NamespaceMember::flag::is_read_only),
         });
         Namespace global = Namespace("global", false, true);
         Namespace game = Namespace("game", false, false, {
            NamespaceMember::make_scope_member("betrayal_booting",        OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::betrayal_booting),
            NamespaceMember::make_scope_member("betrayal_penalty",        OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::betrayal_penalty),
            NamespaceMember::make_scope_member("current_round",           OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::current_round),
            NamespaceMember::make_scope_member("death_event_damage_type", OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::death_event_damage_type),
            NamespaceMember::make_scope_member("friendly_fire",           OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::friendly_fire),
            NamespaceMember::make_scope_member("grace_period",            OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::grace_period),
            NamespaceMember::make_scope_member("grenades_on_map",         OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::grenades_on_map),
            NamespaceMember::make_scope_member("indestructible_vehicles", OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::indestructible_vehicles),
            NamespaceMember::make_scope_member("lives_per_round",         OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::lives_per_round),
            NamespaceMember::make_scope_member("loadout_cam_time",        OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::loadout_cam_time),
            NamespaceMember::make_scope_member("misc_unk0_bit3",          OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::misc_0_bit_3),
            NamespaceMember::make_scope_member("powerup_duration_red",    OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::powerup_duration_r),
            NamespaceMember::make_scope_member("powerup_duration_blue",   OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::powerup_duration_b),
            NamespaceMember::make_scope_member("powerup_duration_yellow", OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::powerup_duration_y),
            NamespaceMember::make_scope_member("respawn_growth",          OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::respawn_growth),
            NamespaceMember::make_scope_member("respawn_time",            OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::respawn_time),
            NamespaceMember::make_scope_member("respawn_traits_duration", OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::respawn_traits_time),
            NamespaceMember::make_scope_member("round_limit",             OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::round_limit),
            NamespaceMember::make_scope_member("round_time_limit",        OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::round_time_limit),
            NamespaceMember::make_scope_member("rounds_to_win",           OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::rounds_to_win),
            NamespaceMember::make_scope_member("score_to_win",            OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::score_to_win),
            NamespaceMember::make_scope_member("social_flags_bit_2",      OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::social_flag_2),
            NamespaceMember::make_scope_member("social_flags_bit_3",      OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::social_flag_3),
            NamespaceMember::make_scope_member("social_flags_bit_4",      OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::social_flag_4),
            NamespaceMember::make_scope_member("sudden_death_time",       OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::sudden_death_time),
            NamespaceMember::make_scope_member("suicide_penalty",         OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::suicide_penalty),
            NamespaceMember::make_scope_member("symmetry",                OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::symmetry),
            NamespaceMember::make_scope_member("symmetry_getter",         OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::symmetry_get),
            NamespaceMember::make_scope_member("team_lives_per_round",    OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::team_lives_per_round),
            NamespaceMember::make_scope_member("teams_enabled",           OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::teams_enabled),
            NamespaceMember::make_scope_member("unkF7A6",                 OpcodeArgValueScalar::typeinfo, Megalo::variable_scope_indicators::number::unkF7A6),
            //
            NamespaceMember::make_scope_member("round_timer",             OpcodeArgValueTimer::typeinfo,  Megalo::variable_scope_indicators::timer::round_timer),
            NamespaceMember::make_scope_member("sudden_death_timer",      OpcodeArgValueTimer::typeinfo,  Megalo::variable_scope_indicators::timer::sudden_death_timer),
            NamespaceMember::make_scope_member("grace_period_timer",      OpcodeArgValueTimer::typeinfo,  Megalo::variable_scope_indicators::timer::grace_period_timer),
         });
      }
   }
}