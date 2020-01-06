#include "trigger.h"
#include "limits.h"
#include "opcode_arg_types/all_indices.h"
#include "../../types/multiplayer.h"

namespace Megalo {
   Trigger::~Trigger() {
      for (auto opcode : this->opcodes)
         delete opcode;
      this->opcodes.clear();
   }
   //
   bool Trigger::read(cobb::ibitreader& stream) noexcept {
      this->blockType.read(stream);
      this->entryType.read(stream);
      if (this->blockType == block_type::for_each_object_with_label)
         this->forgeLabelIndex.read(stream);
      this->raw.conditionStart.read(stream);
      this->raw.conditionCount.read(stream);
      this->raw.actionStart.read(stream);
      this->raw.actionCount.read(stream);
      return true;
   }
   void Trigger::postprocess_opcodes(const std::vector<Condition>& conditions, const std::vector<Action>& actions) noexcept {
      auto& raw = this->raw;
      std::vector<Action*> temp;
      //
      this->opcodes.reserve(raw.actionCount + raw.conditionCount);
      temp.reserve(raw.actionCount);
      //
      if (raw.actionCount > 0) {
         assert(raw.actionStart < actions.size() && "Bad trigger first-action-index."); // TODO: fail with an error in-app instead of asserting
         if (raw.actionStart >= 0) {
            size_t end = raw.actionStart + raw.actionCount;
            if (end <= actions.size()) // TODO: if (end) is too high, fail with an error
               for (size_t i = raw.actionStart; i < end; i++) {
                  auto instance = new Action(actions[i]);
                  this->opcodes.push_back(instance);
                  temp.push_back(instance);
               }
         }
      }
      if (raw.conditionCount > 0) {
         assert(raw.conditionStart < conditions.size() && "Bad trigger first-conditions-index."); // TODO: fail with an error in-app instead of asserting
         if (raw.conditionStart >= 0) {
            size_t end = raw.conditionStart + raw.conditionCount;
            if (end <= conditions.size()) // TODO: if (end) is too high, fail with an error
               for (size_t i = raw.conditionStart; i < end; i++) {
                  auto& condition = conditions[i];
                  if (condition.action >= raw.actionCount) {
                     this->opcodes.push_back(new Condition(condition));
                     continue;
                  }
                  auto target = temp[condition.action];
                  auto it = std::find(this->opcodes.begin(), this->opcodes.end(), target);
                  assert(it != this->opcodes.end() && "Action not present in the opcode list, even though we have to have inserted it?!");
                  this->opcodes.insert(it, new Condition(condition));
               }
         }
      }
   }
   void Trigger::postprocess(GameVariantDataMultiplayer* mp) noexcept {
      if (this->forgeLabelIndex == -1)
         return;
      auto& list = mp->scriptContent.forgeLabels;
      if (this->forgeLabelIndex >= list.size())
         return;
      this->forgeLabel = list[this->forgeLabelIndex].get();
   }
   void Trigger::write(cobb::bitwriter& stream) const noexcept {
      this->blockType.write(stream);
      this->entryType.write(stream);
      if (this->blockType == block_type::for_each_object_with_label) {
         if (!this->forgeLabel)
            this->forgeLabelIndex = -1;
         else
            this->forgeLabelIndex = this->forgeLabel->index;
         this->forgeLabelIndex.write(stream);
      }
      this->raw.conditionStart.write(stream);
      this->raw.conditionCount.write(stream);
      this->raw.actionStart.write(stream);
      this->raw.actionCount.write(stream);
   }
   void Trigger::to_string(const std::vector<std::unique_ptr<Trigger>>& allTriggers, std::string& out, std::string& indent) const noexcept {
      std::string line;
      //
      out += indent;
      out += "Block type: ";
      switch (this->blockType) {
         case block_type::normal:
            out += "normal";
            break;
         case block_type::for_each_object:
            out += "for each object";
            break;
         case block_type::for_each_object_with_label:
            if (!this->forgeLabel) {
               if (this->forgeLabelIndex == -1) {
                  line = "for each object with no label";
               } else {
                  cobb::sprintf(line, "for each object with label #%d", this->forgeLabelIndex);
               }
            } else {
               ReachForgeLabel* f = this->forgeLabel;
               if (!f->name) {
                  cobb::sprintf(line, "label index %u", this->forgeLabelIndex);
                  break;
               }
               cobb::sprintf(line, "for each object with label %s", f->name->english().c_str());
            }
            out += line;
            break;
         case block_type::for_each_player:
            out += "for each player";
            break;
         case block_type::for_each_player_random:
            out += "for each player random?";
            break;
         case block_type::for_each_team:
            out += "for each team";
            break;
         default:
            cobb::sprintf(line, "unknown %u", (uint32_t)this->blockType);
      }
      out += "\r\n";
      //
      out += indent;
      out += "Entry type: ";
      switch (this->entryType) {
         case entry_type::local:             out += "local";          break;
         case entry_type::normal:            out += "normal";         break;
         case entry_type::on_host_migration: out += "host migration"; break;
         case entry_type::on_init:           out += "init";           break;
         case entry_type::on_local_init:     out += "local init";     break;
         case entry_type::on_object_death:   out += "object death";   break;
         case entry_type::pregame:           out += "pregame";        break;
         case entry_type::subroutine:        out += "subroutine";     break;
         default:
            cobb::sprintf(line, "unknown %u", (uint32_t)this->entryType);
      }
      out += "\r\n";
      //
      if (!this->opcodes.size()) {
         out += indent;
         out += "<Empty Trigger>";
      }
      int32_t last_condition_or_group = -1;
      for (auto& opcode : this->opcodes) {
         auto condition = dynamic_cast<const Condition*>(opcode);
         if (condition) {
            if (condition->or_group == last_condition_or_group) {
               cobb::sprintf(line, "%s[ OR] ", indent.c_str());
            } else {
               cobb::sprintf(line, "%s[CND] ", indent.c_str());
            }
            last_condition_or_group = condition->or_group;
            out += line;
            opcode->to_string(line);
            out += line;
            out += "\r\n";
            continue;
         }
         auto action = dynamic_cast<const Action*>(opcode);
         if (action) {
            if (action->function == &actionFunction_runNestedTrigger) {
               cobb::sprintf(line, "%s[ACT] Run nested trigger:\r\n", indent.c_str());
               out += line;
               //
               auto index = dynamic_cast<OpcodeArgValueTrigger*>(action->arguments[0]);
               if (index) {
                  auto i = index->value;
                  if (i < 0 || i >= allTriggers.size()) {
                     cobb::sprintf(line, "%s   <INVALID TRIGGER INDEX %d>\r\n", indent.c_str(), i);
                     out += line;
                     continue;
                  }
                  indent += "   ";
                  line.clear();
                  allTriggers[i]->to_string(allTriggers, line, indent);
                  out += line;
                  indent.resize(indent.size() - 3);
                  continue;
               }
               out += "   <INVALID>\r\n";
               continue;
            }
            cobb::sprintf(line, "%s[ACT] ", indent.c_str());
            out += line;
            opcode->to_string(line);
            out += line;
            out += "\r\n";
            continue;
         }
         out += indent;
         out += "Opcode with unrecognized type!\r\n";
      }
   }
}