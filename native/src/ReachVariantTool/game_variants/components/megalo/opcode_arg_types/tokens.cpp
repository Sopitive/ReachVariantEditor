#include "tokens.h"
#include "../../../types/multiplayer.h"
#include "variables/object.h"
#include "variables/player.h"
#include "variables/number.h"
#include "variables/team.h"
#include "variables/timer.h"

namespace Megalo {
   bool OpcodeStringToken::read(cobb::ibitreader& stream, GameVariantDataMultiplayer& mp) noexcept {
      this->type.read(stream);
      switch (this->type) {
         case OpcodeStringTokenType::none:
            break;
         case OpcodeStringTokenType::player:
            this->value = new OpcodeArgValuePlayer();
            break;
         case OpcodeStringTokenType::team:
            this->value = new OpcodeArgValueTeam();
            break;
         case OpcodeStringTokenType::object:
            this->value = new OpcodeArgValueObject();
            break;
         case OpcodeStringTokenType::number:
         //case OpcodeStringTokenType::number_with_sign:
            this->value = new OpcodeArgValueScalar();
            break;
         case OpcodeStringTokenType::timer:
            this->value = new OpcodeArgValueTimer();
            break;
         default:
            return false;
      }
      if (this->value) {
         this->value->read(stream, mp);
      }
      return true;
   }
   void OpcodeStringToken::write(cobb::bitwriter& stream) const noexcept {
      this->type.write(stream);
      if (this->value)
         this->value->write(stream);
   }
   void OpcodeStringToken::to_string(std::string& out) const noexcept {
      if (this->value)
         this->value->to_string(out);
   }
   void OpcodeStringToken::decompile(Decompiler& out, Decompiler::flags_t flags) noexcept {
      if (this->value)
         this->value->decompile(out, flags);
   }
   //
   //
   //
   OpcodeArgTypeinfo OpcodeArgValueStringTokens2::typeinfo = OpcodeArgTypeinfo(
      "_format_string",
      "Formatted String",
      "A format string and up to two tokens to insert into it.",
      //
      OpcodeArgTypeinfo::flags::none,
      OpcodeArgTypeinfo::default_factory<OpcodeArgValueStringTokens2>
   );
   //
   bool OpcodeArgValueStringTokens2::read(cobb::ibitreader& stream, GameVariantDataMultiplayer& mp) noexcept {
      MegaloStringIndexOptional index;
      index.read(stream);
      this->string = mp.scriptData.strings.get_entry(index);
      //
      this->tokenCount.read(stream);
      if (this->tokenCount > max_token_count) {
         printf("Tokens value claimed to have %d tokens; max is %d.\n", this->tokenCount.to_int(), max_token_count);
         return false;
      }
      for (uint8_t i = 0; i < this->tokenCount; i++)
         this->tokens[i].read(stream, mp);
      return true;
   }
   void OpcodeArgValueStringTokens2::write(cobb::bitwriter& stream) const noexcept {
      MegaloStringIndexOptional index = -1;
      if (this->string)
         index = this->string->index;
      index.write(stream);
      //
      this->tokenCount.write(stream);
      for (uint8_t i = 0; i < this->tokenCount; i++)
         this->tokens[i].write(stream);
   }
   void OpcodeArgValueStringTokens2::to_string(std::string& out) const noexcept {
      if (this->tokenCount == 0) {
         if (!this->string) {
            out = "no string";
            return;
         }
         cobb::sprintf(out, "localized string ID %d", this->string->index);
         return;
      }
      out.clear();
      for (uint8_t i = 0; i < this->tokenCount; i++) {
         std::string temp;
         //
         auto& token = this->tokens[i];
         token.to_string(temp);
         //
         if (!out.empty())
            out += ", ";
         out += temp;
      }
      cobb::sprintf(out, "format string ID %d and tokens: %s", this->string->index, out.c_str());
   }
   void OpcodeArgValueStringTokens2::decompile(Decompiler& out, Decompiler::flags_t flags) noexcept {
      ReachString* format = this->string;
      if (format) {
         std::string english = format->english();
         //
         // Escape ", \r, \n, etc.:
         //
         size_t size = english.size();
         for (size_t i = 0; i < size; ++i) {
            const char* replace = nullptr;
            char c = english[i];
            switch (c) {
               case '\r': replace = "\\r"; break;
               case '\n': replace = "\\n"; break;
               case '"': replace = "\\\""; break;
               case '\\': replace = "\\\\"; break;
            }
            if (replace) {
               english.replace(i, 1, replace);
               i += strlen(replace) - 1;
               continue;
            }
            if (c < '!' && c != ' ') { // TODO: UTF-8 printable char check instead of this
               std::string temp;
               cobb::sprintf(temp, "\\X%02X", (uint32_t)c);
               english.replace(i, 1, temp);
               i += temp.size() - 1;
               continue;
            }
         }
         //
         out.write('"');
         out.write(english);
         out.write('"');
      } else {
         out.write("none");
      }
      //
      for (uint8_t i = 0; i < this->tokenCount; ++i) {
         out.write(", ");
         this->tokens[i].decompile(out, flags);
      }
   }
}