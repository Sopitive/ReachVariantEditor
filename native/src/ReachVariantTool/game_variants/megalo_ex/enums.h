#pragma once
#include <cstdint>
#include <type_traits>
#include "../../helpers/bitwise.h"

namespace std {
   class string;
}

namespace Megalo {
   class SmartEnum {
      public:
         constexpr SmartEnum(const char** v, uint32_t n) : values(v), count(n) {}
         //
         const char** values = nullptr;
         uint32_t     count  =  0;
         //
         constexpr inline int count_bits() const noexcept {
            if (!this->count)
               return 0;
            return cobb::bitcount(this->count);
         }
         constexpr inline int index_bits() const noexcept {
            if (!this->count)
               return 0;
            return cobb::bitcount(this->count - 1);
         }
         //
         void to_string(std::string& out, uint32_t value) const noexcept;
         //
         const char* operator[](int a) const noexcept {
            if (a < this->count)
               return this->values[a];
            return nullptr;
         }
   };
   #define megalo_define_smart_enum(name, ...) namespace { namespace __smart_enum_members { const char* _##name##[] = { __VA_ARGS__ }; } }; extern const ::Megalo::SmartEnum name = ::Megalo::SmartEnum(__smart_enum_members::_##name##, std::extent<decltype(__smart_enum_members::_##name##)>::value);

   class SmartFlags {
      public:
         constexpr SmartFlags(const char** v, uint32_t n) : values(v), count(n) {}
         //
         const char** values = nullptr;
         uint32_t     count  =  0;
         //
         constexpr inline int count_bits() const noexcept {
            if (!this->count)
               return 0;
            return cobb::bitcount(this->count);
         }
         constexpr inline int index_bits() const noexcept {
            if (!this->count)
               return 0;
            return cobb::bitcount(this->count - 1);
         }
         //
         void to_string(std::string& out, uint32_t value) const noexcept;
   };
   #define megalo_define_smart_flags(name, ...) namespace { namespace __smart_flags_members { const char* _##name##[] = { __VA_ARGS__ }; } }; extern const ::Megalo::SmartFlags name = ::Megalo::SmartFlags(__smart_flags_members::_##name##, std::extent<decltype(__smart_flags_members::_##name##)>::value);
}