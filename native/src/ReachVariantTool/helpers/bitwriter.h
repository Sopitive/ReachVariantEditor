#pragma once
#include <cstdint>
#include <type_traits>
#include "bitwise.h"
#include "endianness.h"
#include "polyfills_cpp20.h"
#include "templating.h"

namespace cobb {
   class bitwriter {
      protected:
         enum class _is_signed_sentinel {};
      public:
         static constexpr _is_signed_sentinel is_signed = _is_signed_sentinel();
      protected:
         uint8_t* _buffer = nullptr;
         uint32_t _size   = 0;
         uint32_t _bitpos = 0;
         //
         void _ensure_room_for(unsigned int bitcount) noexcept;
         void _write(uint64_t value, int bits, int& recurse_remaining) noexcept; // should naturally write in big-endian
         //
      public:
         ~bitwriter();
         //
         inline uint32_t get_bitpos()   const noexcept { return this->_bitpos; };
         inline uint32_t get_bytepos()  const noexcept { return this->_bitpos / 8; };
         inline int      get_bitshift() const noexcept { return this->_bitpos % 8; };
         //
         uint8_t get_byte(uint32_t bytepos) const noexcept {
            if (bytepos > this->_bitpos / 8)
               return 0;
            return *(uint8_t*)(this->_buffer + bytepos);
         }
         //
         void dump_to_console() const noexcept;
         inline void enlarge_by(uint32_t bytes) noexcept {
            this->resize(this->_size + bytes);
         }
         inline void pad_bytes(uint32_t bytes) noexcept {
            while (bytes--)
               this->write(0, 8);
         }
         void pad_to_bytepos(uint32_t bytepos) noexcept;
         void resize(uint32_t size) noexcept;
         inline void write(int64_t value, int bits, bool is_signed = false) noexcept {
            if (is_signed && value < 0)
               value |= ((int64_t)1 << (bits - 1));
            this->_write(value, bits, bits);
         }
         //
         template<typename T, cobb_enable_case(1, !std::is_bounded_array_v<T> && std::is_integral_v<T>)> void write(const T& value, const cobb::endian_t save_endianness = cobb::endian_t::little) noexcept {
            T v = value;
            if (save_endianness != cobb::endian::big)
               v = cobb::to_big_endian(v);
            this->write((uint64_t)v, cobb::bits_in<T>);
         };
         template<typename T, cobb_enable_case(2, std::is_bounded_array_v<T>)> void write(const T& value, const cobb::endian_t save_endianness = cobb::endian_t::little) noexcept {
            using item_type = std::remove_extent_t<T>; // from X[i] to X
            //
            if (save_endianness != cobb::endian::big)
               for (int i = 0; i < std::extent<T>::value; i++)
                  this->write(cobb::to_big_endian(value[i]), cobb::bits_in<item_type>);
            else
               for (int i = 0; i < std::extent<T>::value; i++)
                  this->write(value[i], cobb::bits_in<item_type>);
         };
         template<int = 0> void write(const float& value, const cobb::endian_t save_endianness = cobb::endian_t::little) noexcept {
            union {
               uint32_t i;
               float    f = value;
            } uv;
            uint32_t v = uv.i;
            if (save_endianness != cobb::endian::big)
               v = cobb::to_big_endian(v);
            this->write(v, 32);
         }
         template<int = 0> void write(const bool& value, const cobb::endian_t save_endianness = cobb::endian_t::little) noexcept {
            this->write(value ? 1 : 0, 1);
         }
         //
         void write_string(const char* value, int maxlength) noexcept { // writes as bits; stops early after null char
            for (int i = 0; i < maxlength; i++) {
               this->write(value[i]);
               if (!value[i])
                  break;
            }
         }
         void write_wstring(const wchar_t* value, int maxlength, const cobb::endian_t save_endianness = cobb::endian_t::big) noexcept { // writes as bits; stops early after null char
            for (int i = 0; i < maxlength; i++) {
               this->write(value[i], save_endianness);
               if (!value[i])
                  break;
            }
         }
   };
}