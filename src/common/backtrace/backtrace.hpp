/** Copyright 2020 Alibaba Group Holding Limited.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef SRC_COMMON_BACKTRACE_BACKTRACE_HPP_
#define SRC_COMMON_BACKTRACE_BACKTRACE_HPP_

#include <cxxabi.h>

#ifdef WITH_LIBUNWIND
#include <libunwind.h>
#endif

#include <iomanip>
#include <limits>
#include <memory>
#include <ostream>

namespace vineyard {

struct backtrace_info {
 public:
  static void backtrace(std::ostream& _out,
                        bool const compact = false) noexcept {
#ifdef WITH_LIBUNWIND
    thread_local char symbol[1024];
    unw_cursor_t cursor;
    unw_context_t context;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    _out << std::hex << std::uppercase;
    while (0 < unw_step(&cursor)) {
      unw_word_t ip = 0;
      unw_get_reg(&cursor, UNW_REG_IP, &ip);
      if (ip == 0) {
        break;
      }
      unw_word_t sp = 0;
      unw_get_reg(&cursor, UNW_REG_SP, &sp);
      print_reg(_out, ip);
      _out << ": (SP:";
      print_reg(_out, sp);
      _out << ") ";
      unw_word_t offset = 0;
      if (unw_get_proc_name(&cursor, symbol, sizeof(symbol), &offset) == 0) {
        char *buffer = get_demangled_name(symbol);
        if (buffer) {
          _out << "(" << get_demangled_name(symbol) << " + 0x" << offset << ")\n";
          std::free(buffer);
        } else {
          _out << "(" << symbol << " + 0x" << offset << ")\n";
        }
        if (!compact) {
          _out << "\n";
        }
      } else {
        _out << "-- error: unable to obtain symbol name for this frame\n\n";
      }
    }
    _out << std::flush;
#endif
  }

  static char* get_demangled_name(char const* const symbol) noexcept {
    // thread_local std::unique_ptr<char, decltype(std::free)&> demangled_name{
    //     nullptr, std::free};
    // reuse the output buffer, for how `__cxa_demangle` works, see:
    // https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
    // thread_local size_t buffer_length = 0;
    if (!symbol) {
      return nullptr;
    }
    int status = -4;
    char *buffer = abi::__cxa_demangle(symbol, nullptr, nullptr, &status);
    // size_t buffer_length_next = buffer_length;
    // demangled_name.reset(abi::__cxa_demangle(symbol, demangled_name.release(),
    //                                          &buffer_length_next, &status));
    if (status == 0) {
      return buffer;
    } else {
      return nullptr;
    }
  }

 private:
#ifdef WITH_LIBUNWIND
  static void print_reg(std::ostream& _out, unw_word_t reg) noexcept {
    constexpr std::size_t address_width =
        std::numeric_limits<std::uintptr_t>::digits / 4;
    _out << "0x" << std::setfill('0') << std::setw(address_width) << reg;
  }
#endif
};

}  // namespace vineyard

#endif  // SRC_COMMON_BACKTRACE_BACKTRACE_HPP_
