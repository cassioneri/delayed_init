/*******************************************************************************
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 * this software, either in source code form or as a compiled binary, for any
 * purpose, commercial or non-commercial, and by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors of this
 * software dedicate any and all copyright interest in the software to the
 * public domain. We make this dedication for the benefit of the public at large
 * and to the detriment of our heirs and successors. We intend this dedication
 * to be an overt act of relinquishment in perpetuity of all present and future
 * rights to this software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <http://unlicense.org/>
 *
 * If you use this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * by Cassio Neri
 ******************************************************************************/

 /**
  * Unit tests of overload::delayed_init.
  *
  * Tests use the C/C++ standard macro assert and hence diagnostics are fairly
  * poor. More advanced diagnostics can be obtained by using a good unit testing
  * framework as CATCH:
  * http://www.catch-lib.net/
  */

#include <cassert>
#include <iostream>
#include <stack>

#include "delayed_init.h"

// Helper class for testing delayed_init.
class helper {

  int i_;
  
public:

  // List of helper's methods (used for instrumentation purposes).
  enum method {
    none, default_constructor, constructor, copy_constructor,
    move_constructor, destructor, copy_assignment, move_assignment, equal,
    const_get, non_const_get, swap_member, swap_non_member
  };

  // Call stack registering calls to helper's methods.
  static std::stack<method> call_stack;
  
  // Mark call stack by pushing helper::none.
  static void mark_call_stack() {
    call_stack.push(none);
  }

  // Check top of call stack against expectation.
  static void check_call_stack(std::initializer_list<method> methods) {
    for (auto f : methods) {
      assert(call_stack.top() == f);
      call_stack.pop();
    }
    assert(call_stack.top() == none);
    call_stack.pop();
  }
   
  helper() noexcept : i_(0) {
    call_stack.push(default_constructor);
  }

  explicit helper(int i) noexcept : i_(i) {
    call_stack.push(constructor);
  }

  helper(const helper& other) noexcept : i_(other.i_) {
    call_stack.push(copy_constructor);
  }
  
  helper(helper&& other) noexcept : i_(other.i_) {
    call_stack.push(move_constructor);
  }
  
  ~helper() noexcept {
    call_stack.push(destructor);
  }
  
  helper& operator=(const helper& other) noexcept {
    call_stack.push(copy_assignment);
    i_ = other.i_;
    return *this;
  }
  
  helper& operator=(helper&& other) noexcept {
    call_stack.push(move_assignment);
    i_ = other.i_;
    return *this;
  }
  
  bool operator==(const helper& other) const noexcept {
    call_stack.push(equal);
    return i_ == other.i_;
  }
  
  const int& get() const noexcept {
    call_stack.push(const_get);
    return i_;
  }
  
  int& get() noexcept {
    call_stack.push(non_const_get);
    return i_;
  }
  
  void swap(helper& other) noexcept {
    call_stack.push(swap_member);
    std::swap(i_, other.i_);
  }
  
}; // class helper

std::stack<helper::method> helper::call_stack;
using overload::delayed_init;
typedef std::initializer_list<helper::method> method_list;

//------------------------------------------------------------------------------
// swap()
//------------------------------------------------------------------------------

void swap(helper& h1, helper& h2) noexcept {
  helper::call_stack.push(helper::swap_non_member);
  h1.swap(h2);
}

//------------------------------------------------------------------------------
// is_pointer_to_const()
//------------------------------------------------------------------------------

constexpr bool is_pointer_to_const(const helper*) {
  return true;
}

constexpr bool is_pointer_to_const(helper*) {
  return false;
}

//------------------------------------------------------------------------------
// where()
//------------------------------------------------------------------------------

void where(int line, const char* func) {
  std::cout << "line " << line << " : " << func << std::endl;
}

//------------------------------------------------------------------------------
// test_default_constructor()
//------------------------------------------------------------------------------

void test_default_constructor(int line) {
  where(line, __func__);
  helper::mark_call_stack();
  const delayed_init<const helper> d;
  helper::check_call_stack({});
  assert(!d);
}

//------------------------------------------------------------------------------
// test_constructor()
//------------------------------------------------------------------------------

template <typename T, typename U>
void test_constructor(int line, U&& src, bool is_init, method_list ml) {
  where(line, __func__);
  helper::mark_call_stack();
  const delayed_init<T> d(std::forward<U>(src));
  helper::check_call_stack(ml);
  assert(static_cast<bool>(d) == is_init);
}

//------------------------------------------------------------------------------
// test_destructor()
//------------------------------------------------------------------------------

template <typename U>
void test_destructor(int line, const U& src, method_list ml) {
  where(line, __func__);
  {
    const delayed_init<const helper> d(src);
    helper::mark_call_stack();
  }
  helper::check_call_stack(ml);
}

//------------------------------------------------------------------------------
// test_assignment()
//------------------------------------------------------------------------------

template <typename T, typename U>
void test_assignment(int line, U&& from, const T& to, bool is_init,
  method_list ml) {
  
  where(line, __func__);
  delayed_init<helper> d(to);
  helper::mark_call_stack();
  d = std::forward<U>(from);
  helper::check_call_stack(ml);
  assert(static_cast<bool>(d) == is_init);
}

//------------------------------------------------------------------------------
// test_indirection_uninitialised()
//------------------------------------------------------------------------------

template <typename D>
void test_indirection_uninitialised(int line) {

  where(line, __func__);
  D d;
  helper::mark_call_stack();
  try {
    (*d).get();
    assert(false);
  }
  catch (std::logic_error&) {
  }
  catch (...) {
    assert(false);
  }
  helper::check_call_stack({});
}

//------------------------------------------------------------------------------
// test_indirection_initialised()
//------------------------------------------------------------------------------

template <typename D>
void test_indirection_initialised(int line, method_list ml) {
  where(line, __func__);
  D d(helper(1));
  helper::mark_call_stack();
  (*d).get();
  helper::check_call_stack(ml);
}

//------------------------------------------------------------------------------
// test_getter()
//------------------------------------------------------------------------------

template <typename D>
void test_getter(int line, const delayed_init<helper>& src, bool is_const,
  bool is_init) {
  
  where(line, __func__);
  D d(src);
  const auto p = d.get();
  assert(is_pointer_to_const(p) == is_const);
  assert(static_cast<bool>(p) == is_init);
}

//------------------------------------------------------------------------------
// test_dereference()
//------------------------------------------------------------------------------

template <typename D>
void test_dereference(int line, method_list ml) {
  where(line, __func__);
  D d(helper(1));
  helper::mark_call_stack();
  d->get();
  helper::check_call_stack(ml);
}

//------------------------------------------------------------------------------
// test_conversion_to_bool()
//------------------------------------------------------------------------------

void test_conversion_to_bool(int line, const delayed_init<const helper>& d,
  bool is_init) {
  where(line, __func__);
  helper::mark_call_stack();
  assert(static_cast<bool>(d) == is_init);
  helper::check_call_stack({});
}

//------------------------------------------------------------------------------
// test_init_uninitialised()
//------------------------------------------------------------------------------

template <typename... Args>
void test_init_uninitialised(int line, method_list ml, Args&&... args) {
  where(line, __func__);
  delayed_init<const helper> d;
  helper::mark_call_stack();
  d.init(std::forward<Args>(args)...);
  helper::check_call_stack(ml);
  assert(d);
  assert(d.get());
}

//------------------------------------------------------------------------------
// test_init_initialised()
//------------------------------------------------------------------------------

void test_init_initialised(int line) {
  where(line, __func__);
  delayed_init<const helper> d(helper(1));
  helper::mark_call_stack();
  try {
    d.init(1);
    assert(false);
  }
  catch(std::logic_error&) {
  }
  catch (...) {
    assert(false);
  }
  helper::check_call_stack({});
}

//------------------------------------------------------------------------------
// test_swap_member()
//------------------------------------------------------------------------------

template <typename D1, typename D2>
void test_swap_member(int line, D1& d1, D2& d2, method_list ml) {
  where(line, __func__);
  const bool is_init1 = static_cast<bool>(d1);
  const bool is_init2 = static_cast<bool>(d2);
  helper::mark_call_stack();
  d1.swap(d2);
  helper::check_call_stack(ml);
  assert(static_cast<bool>(d2) == is_init1);
  assert(static_cast<bool>(d1) == is_init2);
}

//------------------------------------------------------------------------------
// test_swap_non_member()
//------------------------------------------------------------------------------

template <typename D1, typename D2>
void test_swap_non_member(int line, D1& d1, D2& d2, method_list ml) {
  where(line, __func__);
  const bool is_init1 = static_cast<bool>(d1);
  const bool is_init2 = static_cast<bool>(d2);
  helper::mark_call_stack();
  swap(d1, d2);
  helper::check_call_stack(ml);
  assert(static_cast<bool>(d2) == is_init1);
  assert(static_cast<bool>(d1) == is_init2);
}

//------------------------------------------------------------------------------
// main()
//------------------------------------------------------------------------------

int main() {
  
  /***
   * Test default-constructor.
   */

  test_default_constructor(__LINE__);

  /***
   * Create non-const/const helpers.
   */
   
  helper h;
  const helper ch;
  
  /***
   * Test constructor from helper.
   */

  // lvalue.
  test_constructor<helper>(__LINE__, h, true, {helper::copy_constructor});
  test_constructor<const helper>(__LINE__, h, true,
    {helper::copy_constructor});

  // const lvalue.
  test_constructor<helper>(__LINE__, ch, true, {helper::copy_constructor});
  test_constructor<const helper>(__LINE__, ch, true,
    {helper::copy_constructor});

  // rvalue.
  test_constructor<helper>(__LINE__, std::move(h), true,
    {helper::move_constructor});
  test_constructor<const helper>(__LINE__, std::move(h), true,
    {helper::move_constructor});

  // const rvalue.
  test_constructor<helper>(__LINE__, std::move(ch), true,
    {helper::copy_constructor});
  test_constructor<const helper>(__LINE__, std::move(ch), true,
    {helper::copy_constructor});

  /***
   * Create default/non-default initialised delayed_init non-const/const.
   */
   
  delayed_init<helper> d0;
  delayed_init<const helper> cd0;
  delayed_init<helper> d1(helper(1));
  delayed_init<const helper> cd1(helper(1));
    
  /***
   * Test constructor from delayed_init.
   */

  // uninitialised lvalue.
  test_constructor<helper>(__LINE__, d0, false, {});
  test_constructor<const helper>(__LINE__, d0, false, {});
  
  // uninitialised const lvalue.
  test_constructor<helper>(__LINE__, cd0, false, {});
  test_constructor<const helper>(__LINE__, cd0, false, {});
  
  // initialised lvalue.
  test_constructor<helper>(__LINE__, d1, true, {helper::copy_constructor});
  test_constructor<const helper>(__LINE__, d1, true,
    {helper::copy_constructor});

  // initialised const lvalue.
  test_constructor<helper>(__LINE__, cd1, true, {helper::copy_constructor});
  test_constructor<const helper>(__LINE__, cd1, true,
    {helper::copy_constructor});

  // uninitialised rvalue.
  test_constructor<helper>(__LINE__, std::move(d0), false, {});
  test_constructor<const helper>(__LINE__, std::move(d0), false, {});
  
  // uninitialised const rvalue.
  test_constructor<helper>(__LINE__, std::move(cd0), false, {});
  test_constructor<const helper>(__LINE__, std::move(cd0), false, {});
  
  // initialised rvalue.
  test_constructor<helper>(__LINE__, std::move(d1), true,
    {helper::move_constructor});
  test_constructor<const helper>(__LINE__, std::move(d1), true,
    {helper::move_constructor});

  // initialised const rvalue.
  test_constructor<helper>(__LINE__, std::move(cd1), true,
    {helper::copy_constructor});
  test_constructor<const helper>(__LINE__, std::move(cd1), true,
    {helper::copy_constructor});

  /**
   * Test destructor.
   */
   
  // uninitialised.
  test_destructor(__LINE__, d0, {});
  
  // initialised.
  test_destructor(__LINE__, d1, {helper::destructor});

  /***
   * Test assignment from helper.
   */
   
  // lvalue -> uninitialised.
  test_assignment(__LINE__, h, d0, true, {helper::copy_constructor});
  
  // lvalue -> initialised.
  test_assignment(__LINE__, h, d1, true, {helper::copy_assignment});
  
  // const lvalue -> uninitialised.
  test_assignment(__LINE__, ch, d0, true, {helper::copy_constructor});
  
  // const lvalue -> initialised.
  test_assignment(__LINE__, ch, d1, true, {helper::copy_assignment});

  // rvalue -> uninitialised.
  test_assignment(__LINE__, std::move(h), d0, true,
    {helper::move_constructor});
  
  // rvalue -> initialised.
  test_assignment(__LINE__, std::move(h), d1, true,
    {helper::move_assignment});

  // const rvalue -> uninitialised.
  test_assignment(__LINE__, std::move(ch), d0, true,
    {helper::copy_constructor});

  // const rvalue -> initialised.
  test_assignment(__LINE__, std::move(ch), d1, true,
    {helper::copy_assignment});

  /***
   * Test assignment from uninitialised delayed_init.
   */
   
  // lvalue -> uninitialised.
  test_assignment(__LINE__, d0, d0, false, {});
  
  // lvalue -> initialised.
  test_assignment(__LINE__, d0, d1, false, {helper::destructor});
  
  // const lvalue -> uninitialised.
  test_assignment(__LINE__, cd0, d0, false, {});

  // const lvalue -> initialised.
  test_assignment(__LINE__, cd0, d1, false, {helper::destructor});

  // rvalue -> uninitialised.
  test_assignment(__LINE__, std::move(d0), d0, false, {});

  // rvalue -> initialised.
  test_assignment(__LINE__, std::move(d0), d1, false, {helper::destructor});

  // const rvalue -> uninitialised.
  test_assignment(__LINE__, std::move(cd0), d0, false, {});

  // const rvalue -> initialised.
  test_assignment(__LINE__, std::move(cd0), d1, false,
    {helper::destructor});

  /***
   * Test assignment from initialised delayed_init.
   */
   
  // lvalue -> uninitialised.
  test_assignment(__LINE__, d1, d0, true, {helper::copy_constructor});
  
  // lvalue -> initialised.
  test_assignment(__LINE__, d1, d1, true, {helper::copy_assignment});
  
  // const lvalue -> uninitialised.
  test_assignment(__LINE__, cd1, d0, true, {helper::copy_constructor});

  // const lvalue -> initialised.
  test_assignment(__LINE__, cd1, d1, true, {helper::copy_assignment});

  // rvalue -> uninitialised.
  test_assignment(__LINE__, std::move(d1), d0, true,
    {helper::move_constructor});

  // rvalue -> initialised.
  test_assignment(__LINE__, std::move(d1), d1, true,
    {helper::move_assignment});

  // const rvalue -> uninitialised.
  test_assignment(__LINE__, std::move(cd1), d0, true,
    {helper::copy_constructor});

  // const rvalue -> initialised.
  test_assignment(__LINE__, std::move(cd1), d1, true,
    {helper::copy_assignment});
  
  /***
   * Test indirection for uninitialised delayed_init.
   */

  test_indirection_uninitialised<delayed_init<helper>>(__LINE__);
  test_indirection_uninitialised<delayed_init<const helper>>(__LINE__);
  
  /***
   * Test indirection for initialised delayed_init.
   */

  test_indirection_initialised<delayed_init<helper>>(__LINE__,
    {helper::non_const_get});
  test_indirection_initialised<delayed_init<const helper>>(__LINE__,
    {helper::const_get});

  /**
   * Test getter from uninitialised delayed_init.
   */

  // non-const.
  test_getter<delayed_init<helper>>(__LINE__, d0, false, false);

  // const.
  test_getter<const delayed_init<helper>>(__LINE__, d0, true, false);
  test_getter<delayed_init<const helper>>(__LINE__, d0, true, false);
  test_getter<const delayed_init<const helper>>(__LINE__, d0, true, false);

  /**
   * Test getter from initialised delayed_init.
   */

  // non-const.
  test_getter<delayed_init<helper>>(__LINE__, d1, false, true);

  // const.
  test_getter<delayed_init<const helper>>(__LINE__, d1, true, true);
  test_getter<const delayed_init<helper>>(__LINE__, d1, true, true);
  test_getter<const delayed_init<const helper>>(__LINE__, d1, true, true);

  /***
   * Test dereference.
   */
   
  // non-const.
  test_dereference<delayed_init<helper>>(__LINE__, {helper::non_const_get});

  // const.
  test_dereference<delayed_init<const helper>>(__LINE__,
    {helper::const_get});
  test_dereference<const delayed_init<helper>>(__LINE__,
    {helper::const_get});
  test_dereference<const delayed_init<const helper>>(__LINE__,
    {helper::const_get});

  /***
   * Test conversion to bool.
   */
    
  // uninitialised.
  test_conversion_to_bool(__LINE__, d0, false);
  
  // initialised.
  test_conversion_to_bool(__LINE__, d1, true);
  
  /***
   * Test initialiser.
   */
  
  // lvalue -> uninitialised.
  test_init_uninitialised(__LINE__, {helper::constructor}, 1);
  test_init_uninitialised(__LINE__, {helper::copy_constructor}, h);
  test_init_uninitialised(__LINE__, {helper::copy_constructor}, ch);
  
  // rvalue -> uninitialised.
  test_init_uninitialised(__LINE__, {helper::constructor}, std::move(1));
  test_init_uninitialised(__LINE__, {helper::move_constructor},
    std::move(h));
  test_init_uninitialised(__LINE__, {helper::copy_constructor},
    std::move(ch));
 
  // initialised.
  test_init_initialised(__LINE__);
 
  /***
   * Test swap member.
   */

  // uninitialised <-> uninitialised.
  {
    delayed_init<helper> d1, d2;
    test_swap_member(__LINE__, d1, d2, {});
  }

  // initialised <-> uninitialised.
  {
    delayed_init<helper> d1(h), d2;
    test_swap_member(__LINE__, d1, d2,
      {helper::destructor, helper::move_constructor});
  }
  
  // initialised <-> uninitialised.
  {
    delayed_init<helper> d1, d2(h);
    test_swap_member(__LINE__, d1, d2,
      {helper::destructor, helper::move_constructor});
  }

  // initialised <-> initialised.
  {
    delayed_init<helper> d1(h), d2(h);
    test_swap_member(__LINE__, d1, d2,
      {helper::swap_member, helper::swap_non_member});
  }
  
  /***
   * Test swap non member.
   */

  // uninitialised <-> uninitialised.
  {
    delayed_init<helper> d1, d2;
    test_swap_non_member(__LINE__, d1, d2, {});
  }

  // initialised <-> uninitialised.
  {
    delayed_init<helper> d1(h), d2;
    test_swap_non_member(__LINE__, d1, d2,
      {helper::destructor, helper::move_constructor});
  }
  
  // initialised <-> uninitialised.
  {
    delayed_init<helper> d1, d2(h);
    test_swap_non_member(__LINE__, d1, d2,
      {helper::destructor, helper::move_constructor});
  }

  // initialised <-> initialised.
  {
    delayed_init<helper> d1(h), d2(h);
    test_swap_non_member(__LINE__, d1, d2,
      {helper::swap_member, helper::swap_non_member});
  }

  std::cout << "all tests passed." << std::endl;
  return 0;
}
