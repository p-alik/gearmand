/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Data Differential YATL (i.e. libtest)  library
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <config.h>

#include <libtest/common.h>
  
namespace libtest {

class TestCase {
public:
  TestCase(const std::string& arg):
    _name(arg),
    _result(TEST_FAILURE)
  {
  }

  const std::string& name() const
  {
    return _name;
  }

  test_return_t result() const
  {
    return _result;
  }

  void result(test_return_t arg)
  {
    _result= arg;
  }

  void result(test_return_t arg, const libtest::Timer& timer_)
  {
    _result= arg;
    _timer= timer_;
  }

  const libtest::Timer& timer() const
  {
    return _timer;
  }

  void timer(libtest::Timer& arg)
  {
    _timer= arg;
  }

private:
  std::string _name;
  test_return_t _result;
  libtest::Timer _timer;
};

Formatter::Formatter(const std::string& arg) :
  _suite_name(arg),
  _current_testcase(NULL)
{
}

Formatter::~Formatter()
{
  for (TestCases::iterator iter= testcases.begin(); iter != testcases.end(); ++iter)
  {
    delete *iter;
  }
}

void Formatter::skipped()
{
  assert(_current_testcase);
  _current_testcase->result(TEST_SKIPPED);
  Out << "\tTesting " << _current_testcase->name() <<  "\t\t\t\t\t" << "[ " << test_strerror(_current_testcase->result()) << " ]";

  reset();
}

void Formatter::failed()
{
  assert(_current_testcase);
  _current_testcase->result(TEST_FAILURE);

  Out << "\tTesting " << _current_testcase->name() <<  "\t\t\t\t\t" << "[ " << test_strerror(_current_testcase->result()) << " ]";

  reset();
}

void Formatter::success(const libtest::Timer& timer_)
{
  assert(_current_testcase);
  _current_testcase->result(TEST_SUCCESS, timer_);

  Out << "\tTesting " 
    << _current_testcase->name()
    <<  "\t\t\t\t\t" 
    << _current_testcase->timer() 
    << " [ " << test_strerror(_current_testcase->result()) << " ]";

  reset();
}

void Formatter::push_testcase(const std::string& arg)
{
  assert(_suite_name.empty() == false);
  assert(_current_testcase == NULL);
  _current_testcase= new TestCase(arg);
  testcases.push_back(_current_testcase);
}

void Formatter::reset()
{
  _current_testcase= NULL;
}
} // namespace libtest
