#pragma once
#include "../opcode_arg.h"

namespace Megalo {
   class OpcodeArgValueConstBool : public OpcodeArgValue {
      public:
         static OpcodeArgTypeinfo typeinfo;
         //
      public:
         const char* baseStringTrue  = "true";
         const char* baseStringFalse = "false";
         bool value = false; // loaded value
         //
         virtual bool read(cobb::ibitreader& stream) noexcept override;
         virtual void write(cobb::bitwriter& stream) const noexcept override;
         virtual void to_string(std::string& out) const noexcept override;
         virtual void decompile(Decompiler& out, Decompiler::flags_t flags = Decompiler::flags::none) noexcept override;
         virtual void configure_with_base(const OpcodeArgBase& base) noexcept override;
   };
   class OpcodeArgValueConstSInt8 : public OpcodeArgValue {
      public:
         static OpcodeArgTypeinfo typeinfo;
         //
      public:
         int8_t value = 0; // loaded value
         //
         virtual bool read(cobb::ibitreader& stream) noexcept override;
         virtual void write(cobb::bitwriter& stream) const noexcept override;
         virtual void to_string(std::string& out) const noexcept override;
         virtual void decompile(Decompiler& out, Decompiler::flags_t flags = Decompiler::flags::none) noexcept override;
   };

}