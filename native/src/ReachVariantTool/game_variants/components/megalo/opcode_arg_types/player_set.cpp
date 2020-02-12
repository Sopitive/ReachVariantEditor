#include "player_set.h"
#include "../../../../helpers/strings.h"

namespace Megalo {
   OpcodeArgTypeinfo OpcodeArgValuePlayerSet::typeinfo = OpcodeArgTypeinfo(
      OpcodeArgTypeinfo::typeinfo_type::default,
      0,
      { "no_one", "anyone", "allies", "enemies", "specific_player", "no_one_2" },
      & OpcodeArgValuePlayerSet::factory
   );
   //
   bool OpcodeArgValuePlayerSet::read(cobb::ibitreader& stream) noexcept {
      this->set_type.read(stream);
      if (this->set_type == PlayerSetType::specific_player) {
         return this->player.read(stream) && this->addOrRemove.read(stream);
      }
      return true;
   }
   void OpcodeArgValuePlayerSet::write(cobb::bitwriter& stream) const noexcept {
      this->set_type.write(stream);
      if (this->set_type == PlayerSetType::specific_player) {
         this->player.write(stream);
         this->addOrRemove.write(stream);
      }
   }
   void OpcodeArgValuePlayerSet::to_string(std::string& out) const noexcept {
      if (this->set_type == PlayerSetType::specific_player) {
         std::string temp;
         this->player.to_string(out);
         this->addOrRemove.to_string(temp);
         cobb::sprintf(out, "%s - add or remove: %s", out.c_str(), temp.c_str());
         return;
      }
      switch (this->set_type) {
         case PlayerSetType::no_one:
            out = "no one";
            return;
         case PlayerSetType::anyone:
            out = "anyone";
            return;
         case PlayerSetType::allies:
            out = "allies";
            return;
         case PlayerSetType::enemies:
            out = "enemies";
            return;
         case PlayerSetType::no_one_2:
            out = "no one (?)";
            return;
      }
      cobb::sprintf(out, "unknown:%u", (uint32_t)this->set_type);
   }
}