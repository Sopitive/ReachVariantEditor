#pragma once
#include "../base.h"
#include "../../formats/bitset.h"
#include "../../formats/block.h"
#include "../../formats/content_author.h"
#include "../../formats/localized_string_table.h"
#include "../../helpers/bitnumber.h"
#include "../../helpers/bitwriter.h"
#include "../../helpers/bytewriter.h"
#include "../../helpers/files.h"
#include "../../helpers/indexed_list.h"
#include "../../helpers/pointer_list.h"
#include "../../helpers/stream.h"
#include "../../helpers/standalones/unique_pointer.h"
#include "../components/loadouts.h"
#include "../components/map_permissions.h"
#include "../components/megalo_game_stats.h"
#include "../components/megalo_options.h"
#include "../components/megalo/forge_label.h"
#include "../components/megalo/limits.h"
#include "../components/megalo/trigger.h"
#include "../components/megalo/variable_declarations.h"
#include "../components/megalo/widgets.h"
#include "../components/player_rating_params.h"
#include "../components/player_traits.h"
#include "../components/powerups.h"
#include "../components/teams.h"
#include "../components/tu1_options.h"

struct ReachMPSizeData {
   //
   // Holds information on bit usage.
   //
   ReachMPSizeData();
   struct {
      uint32_t maximum = 0x5028 * 8;
      //
      uint32_t header         = 0; // we include MPVR's block header and SHA-1 stuff in this count despite that existing outside of the MP data class
      uint32_t header_strings = 0;
      uint32_t cg_options     = 0;
      uint32_t team_config    = 0;
      uint32_t script_traits  = 0;
      uint32_t script_options = 0;
      uint32_t script_strings = 0;
      uint32_t option_toggles = 0;
      uint32_t rating_params  = 0;
      uint32_t map_perms      = 0;
      uint32_t script_content = 0;
      uint32_t script_stats   = 0;
      uint32_t script_widgets = 0;
      uint32_t forge_labels   = 0;
      uint32_t title_update_1 = 0;
   } bits;
   struct {
      uint32_t conditions = 0;
      uint32_t actions    = 0;
      uint32_t triggers   = 0;
   } counts;
   //
   void update_from(GameVariant&);
   void update_from(GameVariantDataMultiplayer&);
   void update_script_from(GameVariantDataMultiplayer&);
   //
   uint32_t total_bits() const noexcept;
};

class GameVariantDataMultiplayer : public GameVariantData {
   protected:
      void _set_up_indexed_dummies();
      void _tear_down_indexed_dummies();
      //
      bool _read_script_code(cobb::ibitreader&) noexcept;
      //
   public:
      GameVariantDataMultiplayer(bool isForge) : isForge(isForge) {};
      //
      virtual ReachGameEngine get_type() const noexcept { return this->isForge ? ReachGameEngine::forge : ReachGameEngine::multiplayer; }
      virtual bool read(cobb::reader&) noexcept override;
      virtual void write(GameVariantSaveProcess&) noexcept override;
      virtual void write_last_minute_fixup(GameVariantSaveProcess&) const noexcept override;
      virtual GameVariantData* clone() const noexcept override; // TODO: DOES NOT WORK
      virtual bool receive_editor_data(RVTEditorBlock::subrecord* subrecord) noexcept override;
      //
      static constexpr uint8_t encoding_version_vanilla = 0x6A;
      static constexpr uint8_t encoding_version_tu1     = 0x6B;
      //
      bool isForge = false;
      //
      mutable uint32_t encodingVersion;
      mutable uint32_t engineVersion;
      GameVariantHeader variantHeader;
      cobb::bitbool isBuiltIn;
      struct {
         struct {
            cobb::bitnumber<4, uint8_t> flags; // 0, 1, 2, 3 = perfection enabled, reset players on new round, reset map on new round, teams
            cobb::bytenumber<uint8_t>   timeLimit; // round time limit in minutes
            cobb::bitnumber<5, uint8_t> roundLimit;
            cobb::bitnumber<4, uint8_t> roundsToWin;
            cobb::bitnumber<7, uint8_t> suddenDeathTime; // seconds
            cobb::bitnumber<5, uint8_t> gracePeriod;
         } misc;
         struct {
            cobb::bitnumber<4, uint8_t> flags; // flags: synch with team; unknown (respawn at teammate?); unknown (respawn at location?); respawn on kills
            cobb::bitnumber<6, uint8_t> livesPerRound;
            cobb::bitnumber<7, uint8_t> teamLivesPerRound;
            cobb::bytenumber<uint8_t> respawnTime = 5;
            cobb::bytenumber<uint8_t> suicidePenalty = 5;
            cobb::bytenumber<uint8_t> betrayalPenalty = 5;
            cobb::bitnumber<4, uint8_t> respawnGrowth;
            cobb::bitnumber<4, uint8_t> loadoutCamTime = 10;
            cobb::bitnumber<6, uint8_t> traitsDuration;
            ReachPlayerTraits traits;
         } respawn;
         struct {
            cobb::bitbool observers = false;
            cobb::bitnumber<2, uint8_t> teamChanges; // enum: disabled; enabled; balancing only
            cobb::bitnumber<5, uint8_t> flags; // flags: friendly fire; betrayal booting; proximity voice; global voice; dead player voice
         } social;
         struct {
            cobb::bitnumber<6, uint8_t> flags;
            ReachPlayerTraits baseTraits;
            cobb::bytenumber<int8_t> weaponSet; // map default == -2
            cobb::bytenumber<int8_t> vehicleSet; // map default == -2
            struct {
               ReachPowerupData red;
               ReachPowerupData blue;
               ReachPowerupData yellow;
            } powerups;
         } map;
         struct {
            cobb::bitnumber<3, uint8_t> scoring;
            cobb::bitnumber<3, uint8_t> species;
            cobb::bitnumber<2, uint8_t> designatorSwitchType;
            ReachTeamData teams[8];
         } team;
         struct {
            cobb::bitnumber<2, uint8_t> flags; // flags: spartan loadouts; elite loadouts
            std::array<ReachLoadoutPalette, 6> palettes; // indices: reach::loadout_palette
         } loadouts;
      } options;
      struct {
         cobb::indexed_list<ReachMegaloPlayerTraits, Megalo::Limits::max_script_traits>  traits;
         cobb::indexed_list<ReachMegaloOption,       Megalo::Limits::max_script_options> options;
         ReachStringTable strings = ReachStringTable(112, 0x4C00);
      } scriptData;
      MegaloStringRef  genericName; // more specific than the category name but less specific than the name; could possibly also be a "fallback name"
      ReachStringTable localizedName = ReachStringTable(1, 0x180);
      ReachStringTable localizedDesc = ReachStringTable(1, 0xC00);
      ReachStringTable localizedCategory = ReachStringTable(1, 0x180);
      cobb::bitnumber<cobb::bitcount(32 - 1), int8_t, true> engineIcon;
      cobb::bitnumber<cobb::bitcount(32 - 1), int8_t, true> engineCategory;
      ReachMapPermissions mapPermissions;
      ReachPlayerRatingParams playerRatingParams;
      cobb::bytenumber<uint16_t> scoreToWin;
      cobb::bitbool fireteamsEnabled;
      cobb::bitbool symmetric;
      struct {
         struct {
            ReachGameVariantEngineOptionToggles disabled;
            ReachGameVariantEngineOptionToggles hidden;
         } engine;
         struct {
            ReachGameVariantMegaloOptionToggles disabled;
            ReachGameVariantMegaloOptionToggles hidden;
         } megalo;
      } optionToggles;
      struct {
         struct {
            std::vector<Megalo::Condition> conditions;
            std::vector<Megalo::Action>    actions;
         } raw;
         cobb::indexed_list<Megalo::Trigger, Megalo::Limits::max_triggers> triggers;
         Megalo::TriggerEntryPoints entryPoints;
         cobb::indexed_list<ReachMegaloGameStat, Megalo::Limits::max_script_stats> stats;
         struct {
            Megalo::VariableDeclarationSet global = Megalo::VariableDeclarationSet(Megalo::variable_scope::global);
            Megalo::VariableDeclarationSet player = Megalo::VariableDeclarationSet(Megalo::variable_scope::player);
            Megalo::VariableDeclarationSet object = Megalo::VariableDeclarationSet(Megalo::variable_scope::object);
            Megalo::VariableDeclarationSet team   = Megalo::VariableDeclarationSet(Megalo::variable_scope::team);
         } variables;
         cobb::indexed_list<Megalo::HUDWidgetDeclaration, Megalo::Limits::max_script_widgets> widgets;
         ReachGameVariantUsedMPObjectTypeList usedMPObjectTypes;
         cobb::indexed_list<Megalo::ReachForgeLabel, Megalo::Limits::max_script_labels> forgeLabels;
      } scriptContent;
      ReachGameVariantTU1Options titleUpdateData;
      struct {
         cobb::bitnumber<2, uint8_t> flags = 0;
         cobb::bitnumber<2, uint8_t> editModeType = 0;
         cobb::bitnumber<6, uint8_t> respawnTime = 0;
         ReachPlayerTraits editorTraits;
      } forgeData;

      ReachMegaloOption* create_script_option();
      void delete_script_option(ReachMegaloOption*);
      bool swap_script_options(int8_t index_a, int8_t index_b);

      inline ReachMPSizeData get_size_data() noexcept {
         ReachMPSizeData data;
         data.update_from(*this);
         return data;
      }

      Megalo::ReachForgeLabel* get_forge_label_using_string(ReachString*) const noexcept;
      ReachMegaloOption*       get_option_using_string(ReachString*) const noexcept;
      ReachMegaloPlayerTraits* get_traits_using_string(ReachString*) const noexcept;
};