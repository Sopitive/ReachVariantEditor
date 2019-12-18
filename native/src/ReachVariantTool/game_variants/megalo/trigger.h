#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "actions.h"
#include "conditions.h"
#include "limits.h"
#include "../../helpers/bitstream.h"
#include "../../helpers/bitwise.h"

namespace Megalo {
   enum class block_type : uint8_t {
      //
      // I'm representing Megalo script units as conditions and actions, but Bungie 
      // probably conceived of them as code blocks.
      //
      normal, // not a loop, but trigger can still access loop iterators from containing triggers?
      for_each_player,
      for_each_player_random, // or "for random player X times?"
      for_each_team,
      for_each_object, // every MP object?
      for_each_object_with_label,
   };
   enum class trigger_type : uint8_t {
      normal,
      subroutine, // preserves iterator values?
      on_init,
      on_local_init, // unverified; not used in Bungie gametypes
      on_host_migration, // host migrations and double host migrations
      on_object_death,
      local,
      pregame,
      // Halo 4: incident
   };

   class Trigger {
      public:
         ~Trigger();
         //
         block_type   blockType  = block_type::normal;
         trigger_type entry      = trigger_type::normal;
         int32_t      labelIndex = -1; // Forge label index for block_type::for_each_object_with_label
         struct {
            //
            // Raw data loaded from a game variant file. Reach uses a struct-of-arrays approach to 
            // serialize trigger data, writing all conditions followed by all actions and then headers 
            // for triggers, with each header identifying the start index and count of each opcode type. 
            // This represents raw struct-of-arrays data; the (conditions) and (actions) vectors are 
            // generated post-load by the (postprocess_opcodes) member function.
            //
            int32_t  conditionStart = -1;
            uint32_t conditionCount =  0;
            int32_t  actionStart = -1;
            uint32_t actionCount =  0;
         } raw;
         //
         std::vector<Opcode*> opcodes; // set up by postprocess_opcodes after read; trigger owns the opcodes and deletes them in its destructor
         //
         bool read(cobb::bitstream& stream) noexcept;
         void postprocess_opcodes(const std::vector<Condition>& allConditions, const std::vector<Action>& allActions) noexcept;
         //
         void to_string(const std::vector<Trigger>& allTriggers, std::string& out, std::string& indent) const noexcept; // need the list of all triggers so we can see into Run Nested Trigger actions
         inline void to_string(const std::vector<Trigger>& allTriggers, std::string& out) const noexcept {
            out.clear();
            std::string indent;
            this->to_string(allTriggers, out, indent);
         }
   };

   class TriggerEntryPoints {
      protected:
         static void _stream(cobb::bitstream& stream, int32_t& index) noexcept {
            index = (int32_t)stream.read_bits(cobb::bitcount(Limits::max_triggers)) - 1;
         }
      public:
         static constexpr int32_t none = -1;
         struct {
            int32_t init        = none;
            int32_t localInit   = none;
            int32_t hostMigrate = none;
            int32_t doubleHostMigrate = none;
            int32_t objectDeath = none;
            int32_t local       = none;
            int32_t pregame     = none;
            // Halo 4 only: incident
         } indices;
         //
         bool read(cobb::bitstream& stream) noexcept {
            auto& i = this->indices;
            _stream(stream, i.init);
            _stream(stream, i.localInit);
            _stream(stream, i.hostMigrate);
            _stream(stream, i.doubleHostMigrate);
            _stream(stream, i.objectDeath);
            _stream(stream, i.local);
            _stream(stream, i.pregame);
            return true;
         }
   };
}