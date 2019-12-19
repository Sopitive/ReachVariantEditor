#include "forge_label.h"
//
namespace Megalo {
   void ReachForgeLabel::read(cobb::bitstream& stream) noexcept {
      this->nameIndex.read(stream);
      this->requirements = stream.read_bits(3);
      if (this->requires_object_type())
         this->requiredObjectType = stream.read_bits(cobb::bitcount(Megalo::Limits::max_object_types)) - 1;
      if (this->requires_assigned_team())
         this->requiredTeam = (Megalo::const_team)(stream.read_bits(cobb::bitcount((int)Megalo::const_team::_count - 1)) - 1);
      if (this->requires_number())
         this->requiredNumber = stream.read_bits<uint16_t>(16, cobb::bitstream_read_flags::is_signed);
      this->mapMustHaveAtLeast = stream.read_bits<uint8_t>(7);
   }
   void ReachForgeLabel::postprocess_string_indices(ReachStringTable& table) noexcept {
      this->name = table.get_entry(this->nameIndex);
   }
   void ReachForgeLabel::write(cobb::bitwriter& stream) const noexcept {
      this->nameIndex.write(stream);
      stream.write(this->requirements, 3);
      if (this->requires_object_type())
         stream.write(this->requiredObjectType + 1, cobb::bitcount(Megalo::Limits::max_object_types));
      if (this->requires_assigned_team())
         stream.write((int8_t)this->requiredTeam + 1, cobb::bitcount((int)Megalo::const_team::_count - 1));
      if (this->requires_number())
         stream.write(this->requiredNumber, cobb::endian::big);
      stream.write(this->mapMustHaveAtLeast, 7);
   }
}