/* Proposed SG14 status_code
(C) 2018 - 2019 Niall Douglas <http://www.nedproductions.biz/> (5 commits)
File Created: Feb 2018


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
(See accompanying file Licence.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef SYSTEM_ERROR2_CONFIG_HPP
#define SYSTEM_ERROR2_CONFIG_HPP

// < 0.1 each
#include <cassert>
#include <cstddef>  // for size_t
#include <cstdlib>  // for free

// 0.22
#include <type_traits>

// 0.29
#include <atomic>

// 0.28 (0.15 of which is exception_ptr)
#include <exception>  // for std::exception
// <new> includes <exception>, <exception> includes <new>
#include <new>

// 0.01
#include <initializer_list>

#ifndef SYSTEM_ERROR2_CONSTEXPR14
#if defined(STANDARDESE_IS_IN_THE_HOUSE) || __cplusplus >= 201400 || _MSC_VER >= 1910 /* VS2017 */
//! Defined to be `constexpr` when on C++ 14 or better compilers. Usually automatic, can be overriden.
#define SYSTEM_ERROR2_CONSTEXPR14 constexpr
#else
#define SYSTEM_ERROR2_CONSTEXPR14
#endif
#endif

#ifndef SYSTEM_ERROR2_NORETURN
#if defined(STANDARDESE_IS_IN_THE_HOUSE) || (_HAS_CXX17 && _MSC_VER >= 1911 /* VS2017.3 */)
#define SYSTEM_ERROR2_NORETURN [[noreturn]]
#endif
#endif
#if !defined(SYSTEM_ERROR2_NORETURN)
#ifdef __has_cpp_attribute
#if __has_cpp_attribute(noreturn)
#define SYSTEM_ERROR2_NORETURN [[noreturn]]
#endif
#endif
#endif
#if !defined(SYSTEM_ERROR2_NORETURN)
#if defined(_MSC_VER)
#define SYSTEM_ERROR2_NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define SYSTEM_ERROR2_NORETURN __attribute__((__noreturn__))
#else
#define SYSTEM_ERROR2_NORETURN
#endif
#endif
// GCCs before 7 don't grok [[noreturn]] virtual functions, and warn annoyingly
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 7
#undef SYSTEM_ERROR2_NORETURN
#define SYSTEM_ERROR2_NORETURN
#endif

#ifndef SYSTEM_ERROR2_NODISCARD
#if defined(STANDARDESE_IS_IN_THE_HOUSE) || (_HAS_CXX17 && _MSC_VER >= 1911 /* VS2017.3 */)
#define SYSTEM_ERROR2_NODISCARD [[nodiscard]]
#endif
#endif
#ifndef SYSTEM_ERROR2_NODISCARD
#ifdef __has_cpp_attribute
#if __has_cpp_attribute(nodiscard)
#define SYSTEM_ERROR2_NODISCARD [[nodiscard]]
#endif
#elif defined(__clang__)
#define SYSTEM_ERROR2_NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
// _Must_inspect_result_ expands into this
#define SYSTEM_ERROR2_NODISCARD                                                                                                                                                                                                                                                                                                \
  __declspec("SAL_name"                                                                                                                                                                                                                                                                                                        \
             "("                                                                                                                                                                                                                                                                                                               \
             "\"_Must_inspect_result_\""                                                                                                                                                                                                                                                                                       \
             ","                                                                                                                                                                                                                                                                                                               \
             "\"\""                                                                                                                                                                                                                                                                                                            \
             ","                                                                                                                                                                                                                                                                                                               \
             "\"2\""                                                                                                                                                                                                                                                                                                           \
             ")") __declspec("SAL_begin") __declspec("SAL_post") __declspec("SAL_mustInspect") __declspec("SAL_post") __declspec("SAL_checkReturn") __declspec("SAL_end")
#endif
#endif
#ifndef SYSTEM_ERROR2_NODISCARD
#define SYSTEM_ERROR2_NODISCARD
#endif

#ifndef SYSTEM_ERROR2_TRIVIAL_ABI
#if defined(STANDARDESE_IS_IN_THE_HOUSE) || (__clang_major__ >= 7 && !defined(__APPLE__))
//! Defined to be `[[clang::trivial_abi]]` when on a new enough clang compiler. Usually automatic, can be overriden.
#define SYSTEM_ERROR2_TRIVIAL_ABI [[clang::trivial_abi]]
#else
#define SYSTEM_ERROR2_TRIVIAL_ABI
#endif
#endif

#ifndef SYSTEM_ERROR2_NAMESPACE
//! The system_error2 namespace name.
#define SYSTEM_ERROR2_NAMESPACE system_error2
//! Begins the system_error2 namespace.
#define SYSTEM_ERROR2_NAMESPACE_BEGIN                                                                                                                                                                                                                                                                                          \
  namespace system_error2                                                                                                                                                                                                                                                                                                      \
  {
//! Ends the system_error2 namespace.
#define SYSTEM_ERROR2_NAMESPACE_END }
#endif

//! Namespace for the library
SYSTEM_ERROR2_NAMESPACE_BEGIN

//! Namespace for user specialised traits
namespace traits
{
  /*! Specialise to true if you guarantee that a type is move relocating (i.e.
  its move constructor equals copying bits from old to new, old is left in a
  default constructed state, and calling the destructor on a default constructed
  instance is trivial). All trivially copyable types are move relocating by
  definition, and that is the unspecialised implementation.
  */
  template <class T> struct is_move_relocating
  {
    static constexpr bool value = std::is_trivially_copyable<T>::value;
  };
}  // namespace traits

namespace detail
{
  inline SYSTEM_ERROR2_CONSTEXPR14 size_t cstrlen(const char *str)
  {
    const char *end = nullptr;
    for(end = str; *end != 0; ++end)  // NOLINT
      ;
    return end - str;
  }

  /* A partially compliant implementation of C++20's std::bit_cast function contributed
  by Jesse Towner. TODO FIXME Replace with C++ 20 bit_cast when available.

  Our bit_cast is only guaranteed to be constexpr when both the input and output
  arguments are either integrals or enums. However, this covers most use cases
  since the vast majority of status_codes have an underlying type that is either
  an integral or enum. We still attempt a constexpr union-based type pun for non-array
  input types, which some compilers accept. For array inputs, we fall back to
  non-constexpr memmove.
  */

  template <class T> using is_integral_or_enum = std::integral_constant<bool, std::is_integral<T>::value || std::is_enum<T>::value>;

  template <class To, class From> using is_static_castable = std::integral_constant<bool, is_integral_or_enum<To>::value && is_integral_or_enum<From>::value>;

  template <class To, class From> using is_union_castable = std::integral_constant<bool, !is_static_castable<To, From>::value && !std::is_array<To>::value && !std::is_array<From>::value>;

  template <class To, class From> using is_bit_castable = std::integral_constant<bool, sizeof(To) == sizeof(From) && traits::is_move_relocating<To>::value && traits::is_move_relocating<From>::value>;

  template <class To, class From> union bit_cast_union {
    From source;
    To target;
  };

  template <class To, class From,
            typename std::enable_if<                //
            is_bit_castable<To, From>::value        //
            && is_static_castable<To, From>::value  //
            && !is_union_castable<To, From>::value,  //
            bool>::type = true>  //
  constexpr To bit_cast(const From &from) noexcept
  {
    return static_cast<To>(from);
  }

  template <class To, class From,
            typename std::enable_if<                 //
            is_bit_castable<To, From>::value         //
            && !is_static_castable<To, From>::value  //
            && is_union_castable<To, From>::value,    //
            bool>::type = true>  //
  constexpr To bit_cast(const From &from) noexcept
  {
    return bit_cast_union<To, From>{from}.target;
  }

  template <class To, class From,
            typename std::enable_if<                 //
            is_bit_castable<To, From>::value         //
            && !is_static_castable<To, From>::value  //
            && !is_union_castable<To, From>::value,   //
            bool>::type = true>  //
  To bit_cast(const From &from) noexcept
  {
    bit_cast_union<To, From> ret;
    memmove(&ret.source, &from, sizeof(ret.source));
    return ret.target;
  }

  /* erasure_cast performs a bit_cast with additional rules to handle types
  of differing sizes. For integral & enum types, it may perform a narrowing
  or widing conversion with static_cast if necessary, before doing the final
  conversion with bit_cast. When casting to or from non-integral, non-enum
  types it may insert the value into another object with extra padding bytes
  to satisfy bit_cast's preconditions that both types have the same size. */

  template <class To, class From> using is_erasure_castable = std::integral_constant<bool, traits::is_move_relocating<To>::value && traits::is_move_relocating<From>::value>;

  template <class T, bool = std::is_enum<T>::value> struct identity_or_underlying_type
  {
    using type = T;
  };
  template <class T> struct identity_or_underlying_type<T, true>
  {
    using type = typename std::underlying_type<T>::type;
  };

  template <class OfSize, class OfSign>
  using erasure_integer_type = typename std::conditional<std::is_signed<typename identity_or_underlying_type<OfSign>::type>::value, typename std::make_signed<typename identity_or_underlying_type<OfSize>::type>::type, typename std::make_unsigned<typename identity_or_underlying_type<OfSize>::type>::type>::type;

  template <class ErasedType, std::size_t N> struct padded_erasure_object
  {
    static_assert(traits::is_move_relocating<ErasedType>::value, "ErasedType must be TriviallyCopyable or MoveRelocating");
    static_assert(alignof(ErasedType) <= sizeof(ErasedType), "ErasedType must not be over-aligned");
    ErasedType value;
    char padding[N];
    constexpr explicit padded_erasure_object(const ErasedType &v) noexcept
        : value(v)
        , padding{}
    {
    }
  };

  template <class To, class From, typename std::enable_if<is_erasure_castable<To, From>::value && (sizeof(To) == sizeof(From)), bool>::type = true> constexpr To erasure_cast(const From &from) noexcept { return bit_cast<To>(from); }

  template <class To, class From, typename std::enable_if<is_erasure_castable<To, From>::value && is_static_castable<To, From>::value && (sizeof(To) < sizeof(From)), bool>::type = true> constexpr To erasure_cast(const From &from) noexcept { return static_cast<To>(bit_cast<erasure_integer_type<From, To>>(from)); }

  template <class To, class From, typename std::enable_if<is_erasure_castable<To, From>::value && is_static_castable<To, From>::value && (sizeof(To) > sizeof(From)), bool>::type = true> constexpr To erasure_cast(const From &from) noexcept { return bit_cast<To>(static_cast<erasure_integer_type<To, From>>(from)); }

  template <class To, class From, typename std::enable_if<is_erasure_castable<To, From>::value && !is_static_castable<To, From>::value && (sizeof(To) < sizeof(From)), bool>::type = true> constexpr To erasure_cast(const From &from) noexcept
  {
    return bit_cast<padded_erasure_object<To, sizeof(From) - sizeof(To)>>(from).value;
  }

  template <class To, class From, typename std::enable_if<is_erasure_castable<To, From>::value && !is_static_castable<To, From>::value && (sizeof(To) > sizeof(From)), bool>::type = true> constexpr To erasure_cast(const From &from) noexcept
  {
    return bit_cast<To>(padded_erasure_object<From, sizeof(To) - sizeof(From)>{from});
  }
}  // namespace detail
SYSTEM_ERROR2_NAMESPACE_END

#ifndef SYSTEM_ERROR2_FATAL
#include <cstdlib>  // for abort
#ifdef __APPLE__
#include <unistd.h>  // for write
#endif

SYSTEM_ERROR2_NAMESPACE_BEGIN
namespace detail
{
  namespace avoid_stdio_include
  {
#ifndef __APPLE__
    extern "C" ptrdiff_t write(int, const void *, size_t);
#endif
  }  // namespace avoid_stdio_include
  inline void do_fatal_exit(const char *msg)
  {
    using namespace avoid_stdio_include;
    write(2 /*stderr*/, msg, cstrlen(msg));
    write(2 /*stderr*/, "\n", 1);
    abort();
  }
}  // namespace detail
SYSTEM_ERROR2_NAMESPACE_END
//! Prints msg to stderr, and calls `std::terminate()`. Can be overriden via predefinition.
#define SYSTEM_ERROR2_FATAL(msg) ::SYSTEM_ERROR2_NAMESPACE::detail::do_fatal_exit(msg)
#endif

#endif
