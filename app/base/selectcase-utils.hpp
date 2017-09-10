/* GIMP-painter
 * selectcase-utils
 * Copyright (C) 2017  seagetch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef APP_BASE_SELECTCASE_UTILS_HPP_
#define APP_BASE_SELECTCASE_UTILS_HPP_

#include "glib-cxx-utils.hpp"
#include <string.h>

template<typename Matcher>
class SelectCase {
  bool matched;
  Matcher matcher;

  template<typename F>
  SelectCase& when_ (F f) {
    return *this;
  }

public:
  SelectCase(typename Matcher::test_type s) : matcher(s), matched(false) { };

  template<typename F>
  SelectCase& when(typename Matcher::pattern_type pattern, F f) {
    if (!matched) {
      matched = matcher.test(pattern, f);
    }
    return *this;
  };

  template<typename F>
  SelectCase& when(GLib::Array patterns, F f) {
    GLib::IArray<typename Matcher::pattern_type> i_patterns = patterns.ptr();
    for (auto i: i_patterns) {
      if (matched)
        break;
      when (i, f);
    }
    return *this;
  };

  template<typename F>
  void otherwise(F callback) {
    if (!matched)
      callback();
  };

  template<typename... Args>
  static GArray* one_of(Args... args) {
    auto pattern_type = GLib::IArray<typename Matcher::pattern_type>::_new(args...);
    return pattern_type.ptr();
  }

};


class RegexMatcher {
  GLib::CString str;
public:
  using test_type    = const gchar*;
  using pattern_type = GLib::Regex;

  RegexMatcher(const test_type t) : str(g_strdup(t)) {};

  template<typename F>
  bool test(pattern_type& pattern, F matched_callback) {
    return pattern.match(str, matched_callback);
  }
};
using regex_case = SelectCase<RegexMatcher>;


class GTypeMatcher {
  GType type;
public:
  using test_type    = GType;
  using pattern_type = GType;

  GTypeMatcher(const test_type t) : type(t) {};

  template<typename F>
  bool test(pattern_type& pattern, F matched_callback) {
    bool result = (type == pattern);
    if (result) matched_callback();
    return result;
  }
};
using gtype_case = SelectCase<GTypeMatcher>;


class CStringMatcher {
  GLib::CString str;
public:
  using test_type    = const gchar*;
  using pattern_type = const gchar*;

  CStringMatcher(const test_type t) : str(g_strdup(t)) {};

  template<typename F>
  bool test(pattern_type& pattern, F matched_callback) {
    bool result = (strcmp((const gchar*)str, pattern) == 0);
    if (result) matched_callback();
    return result;
  }
};
using string_case = SelectCase<CStringMatcher>;

#endif /* APP_BASE_SELECTCASE_UTILS_HPP_ */
