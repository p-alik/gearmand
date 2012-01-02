/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  libtest
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#pragma once

#include <cstring>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>

namespace libtest {

typedef std::vector<char> _vchar_t;

class vchar_t {
public:
  vchar_t()
  {
  }

  vchar_t(const char *arg, const size_t arg_size)
  {
    _vchar.resize(arg_size);
    memcpy(&_vchar[0], arg, arg_size);
  }

  const _vchar_t& operator*() const
  { 
    return _vchar;
  }

  _vchar_t& operator*()
  { 
    return _vchar;
  }

  const _vchar_t& get() const
  { 
    return _vchar;
  }

  _vchar_t& get()
  { 
    return _vchar;
  }

  bool operator==(const vchar_t& right) const;

  bool operator!=(const vchar_t& right) const;

  ~vchar_t()
  {
  }

private:
  _vchar_t _vchar;
};

std::ostream& operator<<(std::ostream& output, const libtest::vchar_t& arg);

} // namespace libtest

