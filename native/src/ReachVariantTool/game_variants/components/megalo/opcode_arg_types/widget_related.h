#pragma once
#include "../opcode_arg.h"
#include "scalar.h"
#include "timer.h"
#include "../limits.h"
#include "../widgets.h"

class GameVariantDataMultiplayer;

namespace Megalo {
   class OpcodeArgValueWidget : public OpcodeArgValue {
      public:
         static constexpr int16_t index_of_none = -1;
         static constexpr int16_t max_index     = Limits::max_script_widgets;
         //
         int16_t                   index = index_of_none;
         ref<HUDWidgetDeclaration> value = decltype(value)::make(*this);
         bool postprocessed = false;
         //
         static OpcodeArgValue* factory(cobb::ibitreader&) {
            return new OpcodeArgValueWidget;
         }
         virtual bool read(cobb::ibitreader& stream) noexcept override;
         virtual void write(cobb::bitwriter& stream) const noexcept override;
         virtual void postprocess(GameVariantDataMultiplayer* newlyLoaded) noexcept override;
         //
         virtual void to_string(std::string& out) const noexcept override;
   };

   enum class MeterType {
      none,
      timer,
      number,
   };
   class OpcodeArgValueMeterParameters : public OpcodeArgValue {
      public:
         cobb::bitnumber<2, MeterType> type = MeterType::none;
         OpcodeArgValueTimer  timer;
         OpcodeArgValueScalar numerator;
         OpcodeArgValueScalar denominator;
         //
         virtual bool read(cobb::ibitreader& stream) noexcept override;
         virtual void write(cobb::bitwriter& stream) const noexcept override;
         virtual void to_string(std::string& out) const noexcept override;
         //
         static OpcodeArgValue* factory(cobb::ibitreader&) {
            return new OpcodeArgValueMeterParameters;
         }
   };
}