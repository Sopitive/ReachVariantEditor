#include "conditions.h"
#include "opcode_arg_types/all.h"

namespace Megalo {
   extern std::array<ConditionFunction, 18> conditionFunctionList = {{
      //
      // The double-curly-braces for this array are NOT a mistake; you should be able to 
      // use single braces but not every compiler handles that correctly.
      //
      ConditionFunction( // 0
         "None",
         "This condition does nothing.",
         "None.",
         {}
      ),
      ConditionFunction( // 1
         "Compare",
         "Compares any two values.",
         "%1 %v %3 %2.",
         {
            OpcodeArgBase("a", OpcodeArgAnyVariableFactory),
            OpcodeArgBase("b", OpcodeArgAnyVariableFactory),
            OpcodeArgBase("operator", OpcodeArgValueCompareOperatorEnum::factory)
         }
      ),
      ConditionFunction( // 2
         "Object In Boundary",
         "Checks whether one object is inside of another object's Shape.",
         "%1 %v inside of %2's shape.",
         {
            OpcodeArgBase("a", OpcodeArgValueObject::factory),
            OpcodeArgBase("b", OpcodeArgValueObject::factory),
         }
      ),
      ConditionFunction( // 3
         "Killer Type",
         "Checks what killed a player.",
         "%1 %v killed by any of: %2.",
         {
            OpcodeArgBase("victim", OpcodeArgValuePlayer::factory),
            OpcodeArgBase("killer types", OpcodeArgValueKillerTypeFlags::factory),
         },
         "was", "was not"
      ),
      ConditionFunction( // 4
         "Team Disposition",
         "",
         "%1 %v disposition %3 with %2.",
         {
            OpcodeArgBase("a", OpcodeArgValueTeam::factory),
            OpcodeArgBase("b", OpcodeArgValueTeam::factory),
            OpcodeArgBase("disposition", OpcodeArgValueTeamDispositionEnum::factory),
         },
         "has", "does not have"
      ),
      ConditionFunction( // 5
         "Timer Is Zero",
         "Checks whether a timer is at zero.",
         "%1 %v at zero.",
         {
            OpcodeArgBase("timer", OpcodeArgValueTimer::factory),
         }
      ),
      ConditionFunction( // 6
         "Object Type",
         "Checks an object's type.",
         "%1 %v of type %2.",
         {
            OpcodeArgBase("object", OpcodeArgValueObject::factory),
            OpcodeArgBase("type",   OpcodeArgValueMPObjectTypeIndex::factory),
         }
      ),
      ConditionFunction( // 7
         "Team Has Players",
         "Checks whether a team has one or more players on it.",
         "%1 %v one or more players on it.",
         {
            OpcodeArgBase("team", OpcodeArgValueTeam::factory),
         },
         "has", "does not have"
      ),
      ConditionFunction( // 8
         "Object Out Of Bounds",
         "Checks whether an object is out of bounds.",
         "%1 %v out of bounds.",
         {
            OpcodeArgBase("object", OpcodeArgValueObject::factory),
         }
      ),
      ConditionFunction( // 9
         "Deprecated-09",
         "This condition always returns false.", // TODO: does inverting it change that?
         "Never. (Unused argument: %1)",
         {
            OpcodeArgBase("player", OpcodeArgValuePlayer::factory),
         }
      ),
      ConditionFunction( // 10
         "Player Assisted Kill Of",
         "Checks whether one player assisted in the slaying of another player. Note that players don't count as \"assisting\" themselves.",
         "%1 %v in the killing of %2.",
         {
            OpcodeArgBase("attacker", OpcodeArgValuePlayer::factory),
            OpcodeArgBase("victim",   OpcodeArgValuePlayer::factory),
         },
         "assisted", "did not assist"
      ),
      ConditionFunction( // 11
         "Object Has Label",
         "Checks whether an object has a Forge label.",
         "%1 %v label %2.",
         {
            OpcodeArgBase("object", OpcodeArgValueObject::factory),
            OpcodeArgBase("label",  OpcodeArgValueLabelIndex::factory),
         },
         "has", "does not have"
      ),
      ConditionFunction( // 12
         "Player Is Not Respawning",
         "Checks whether a player is NOT waiting to respawn.", // TODO: includes loadout cam time?
         "%1 %v waiting to respawn.",
         {
            OpcodeArgBase("player", OpcodeArgValuePlayer::factory),
         },
         "is not", "is"
      ),
      ConditionFunction( // 13
         "Equipment In Use",
         "",
         "%1 %v in use.",
         {
            OpcodeArgBase("object", OpcodeArgValueObject::factory),
         }
      ),
      ConditionFunction( // 14
         "Species Is Spartan",
         "Checks whether a player is a Spartan.",
         "%1 %v a Spartan.",
         {
            OpcodeArgBase("player", OpcodeArgValuePlayer::factory),
         }
      ),
      ConditionFunction( // 15
         "Species Is Elite",
         "Checks whether a player is an Elite.",
         "%1 %v an Elite.",
         {
            OpcodeArgBase("player", OpcodeArgValuePlayer::factory),
         }
      ),
      ConditionFunction( // 16
         "Species Is Monitor",
         "Checks whether a player is a Monitor.",
         "%1 %v a Monitor.",
         {
            OpcodeArgBase("player", OpcodeArgValuePlayer::factory),
         }
      ),
      ConditionFunction( // 17
         "In Matchmaking",
         "This condition is believed to test whether the current match is taking place in Matchmaking.",
         "This match %v taking place in Matchmaking.",
         {}
      ),
   }};

   bool Condition::read(cobb::bitstream& stream) noexcept {
      {
         auto&  list  = conditionFunctionList;
         size_t index = stream.read_bits<size_t>(cobb::bitcount(list.size() - 1));
         if (index >= list.size()) {
            printf("Bad condition function ID %d.\n", index);
            return false;
         }
         this->function = &list[index];
         if (index == 0) { // The "None" condition loads no further data.
            return true;
         }
      }
      stream.read(this->inverted);
      this->or_group = stream.read_bits(cobb::bitcount(512 - 1));
      this->action   = stream.read_bits(cobb::bitcount(1024 - 1));
      //
      auto&  base     = this->function->arguments;
      size_t argCount = base.size();
      this->arguments.resize(argCount);
      for (size_t i = 0; i < argCount; i++) {
         auto factory = base[i].factory;
         this->arguments[i] = factory(stream);
         if (this->arguments[i]) {
            if (!this->arguments[i]->read(stream)) {
               printf("Failed to load argument %d for condition %s.\n", i, this->function->name);
               return false;
            }
         } else {
            printf("Failed to construct argument %d for condition %s.\n", i, this->function->name);
            return false;
         }
      }
      #if _DEBUG
         this->to_string(this->debug_str);
      #endif
      return true;
   }
   void Condition::to_string(std::string& out) const noexcept {
      if (!this->function) {
         out = "<NO FUNC>";
         return;
      }
      auto f = this->function->format;
      assert(f && "Cannot stringify a condition whose function has no format string!");
      out.clear();
      size_t l = strlen(f);
      for (size_t i = 0; i < l; i++) {
         unsigned char c = f[i];
         if (c != '%') {
            out += c;
            continue;
         }
         c = f[++i];
         if (c == '%') {
            out += '%';
            continue;
         }
         if (c == 'v') {
            out += (this->inverted) ? this->function->verb_invert : this->function->verb_normal;
            continue;
         }
         if (c >= '1' && c <= '9') {
            c -= '1';
            if (c >= this->arguments.size()) {
               out += '%';
               out += (c + '1');
               continue;
            }
            std::string temporary;
            this->arguments[c]->to_string(temporary);
            out += temporary;
         }
      }
   }
}