#pragma once
#include "../opcode_arg.h"
#include "variables/number.h"
#include "variables/timer.h"
#include "../limits.h"
#include "../widgets.h"

class GameVariantDataMultiplayer;

namespace Megalo {
   class OpcodeArgValueWidget : public OpcodeArgValue {
      public:
         static OpcodeArgTypeinfo typeinfo;
         //
      public:
         static constexpr int16_t index_of_none = -1;
         static constexpr int16_t max_index     = Limits::max_script_widgets;
         //
         int16_t index = index_of_none;
         cobb::refcount_ptr<HUDWidgetDeclaration> value;
         bool postprocessed = false;
         //
         virtual bool read(cobb::ibitreader& stream) noexcept override;
         virtual void write(cobb::bitwriter& stream) const noexcept override;
         virtual void postprocess(GameVariantDataMultiplayer* newlyLoaded) noexcept override;
         virtual void to_string(std::string& out) const noexcept override;
         virtual void decompile(Decompiler& out, Decompiler::flags_t flags = Decompiler::flags::none) noexcept override;
   };

   enum class MeterType {
      none,
      timer,
      number,
   };
   class OpcodeArgValueMeterParameters : public OpcodeArgValue {
      public:
         static OpcodeArgTypeinfo typeinfo;
         //
      public:
         cobb::bitnumber<2, MeterType> type = MeterType::none;
         OpcodeArgValueTimer  timer;
         OpcodeArgValueScalar numerator;
         OpcodeArgValueScalar denominator;
         //
         virtual bool read(cobb::ibitreader& stream) noexcept override;
         virtual void write(cobb::bitwriter& stream) const noexcept override;
         virtual void to_string(std::string& out) const noexcept override;
         virtual void decompile(Decompiler& out, Decompiler::flags_t flags = Decompiler::flags::none) noexcept override;
   };
}