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
  * @file delayed_init.h
  * @brief Definition of class delayed_init.
  */

#ifndef OVERLOAD_DELAYED_INIT_H_
#define OVERLOAD_DELAYED_INIT_H_

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace overload {
namespace traits {

using std::swap;

/**
 * @brief Detects if two types are no-throw swappable.
 *
 * More precisely, is_nothrow_swappable<T, U> publicly derives from
 * std::false_type unless a (unqualified) call to swap(T&, U&) is marked as no
 * throw.
 * 
 * @tparam T First type.
 * @tparam U Second type.
 */ 
template <typename T, typename U = T, typename = void>
struct is_nothrow_swappable : public std::false_type {
};

template <typename T, typename U>
struct is_nothrow_swappable<T, U, typename std::enable_if<noexcept(
  swap(std::declval<T&>(),std::declval<U&>()))>::type> : public std::true_type {
};

} // namespace traits

/**
 * @brief Prevents default-initialisation.
 *
 * Class delay_init<T> holds an object of type T whose initialisation is delayed
 * until init() is called.
 *
 * The compiler tries hard to initialise every non-static data member of a class
 * before execution reachs the constructor body. If delaying the initialisation
 * of a non-static data member of type T to a later time is needed, then declare
 * the member as a delayed_init<T> rather than a T.
 *
 * The type T must not be a reference type.
 * 
 * Reference:
 * Cassio Neri, "Complex logic in the member initialiser list", to appear in
 * Overload:
 * http://accu.org/index.php/journals/c78/
 */
template <typename T>
class delayed_init {

public:
  
  static_assert(!std::is_reference<T>::value, "instantiation of delayed_init "
    "for reference type");

  /**
   * @brief Default constructor.
   *
   * @post static_cast<bool>(*this) == false && get() == nullptr.
   * @throw - Nothing.
   */
  constexpr delayed_init() noexcept {
  }

  /**
   * @brief Copy-constructor.
   *
   * If static_cast<bool>(src) == true, then *get() is copy-constructed from
   * *src.get().
   *
   * @post static_cast<bool>(*this) == static_cast<bool>(src) &&
   * (get() == nullptr) == (src.get() == nullptr).
   * @param src Initialiser.
   * @throw - Whatever T::T(const T&) throws.
   */
  delayed_init(const delayed_init& src)
    noexcept(std::is_nothrow_copy_constructible<T>::value) {
    init_me(static_cast<bool>(src), *src.get());
  }

  /**
   * @brief Copy-constructor from delayed_init<U>.
   *
   * If static_cast<bool>(src) == true, then *get() is copy-constructed from
   * *src.get().
   * 
   * @post static_cast<bool>(*this) == static_cast<bool>(src) &&
   * (get() == nullptr) == (src.get() == nullptr).
   * @param src Initialiser.
   * @throw - Whatever T::T(const U&) throws.
   */
  template <typename U>
  delayed_init(const delayed_init<U>& src)
    noexcept(std::is_nothrow_constructible<T, const U&>::value) {
    init_me(static_cast<bool>(src), *src.get());
  }

  /**
   * @brief Move-constructor from delayed_init<U>.
   *
   * If static_cast<bool>(src) == true, then *get() is copy- or move-constructed
   * from *src.get().
   * 
   * @post static_cast<bool>(*this) == static_cast<bool>(src) &&
   * (get() == nullptr) == (src.get() == nullptr).
   * @param src Initialiser.
   * @throw - Whatever T::T(U&&) throw.
   */
  template <typename U>
  delayed_init(delayed_init<U>&& src)
    noexcept(std::is_nothrow_constructible<T, U&&>::value) {
    init_me(static_cast<bool>(src), std::move(*src.get()));
  }
  
  /**
   * @brief Constructor from U.
   *
   * *get() is copy- or move-constructed from obj.
   *
   * @post static_cast<bool>(*this) == true && get() != nullptr.
   * @param obj Initialiser.
   * @throw - Whatever T::T(U&&) throw.
   */
  template <typename U, typename = typename
    std::enable_if<std::is_constructible<T, U&&>::value>::type>
  explicit delayed_init(U&& obj)
    noexcept(std::is_nothrow_constructible<T, U&&>::value) {
    init_obj(std::forward<U>(obj));
  }

  /**
   * @brief Destructor.
   *
   * T:~T() must not throw.
   *
   * @throw - Nothing.
   */
  ~delayed_init() noexcept {
    destroy();
  }

  /**
   * @brief Copy-assignment.
   *
   * If static_cast<bool>(*this) == true and static_cast<bool>(src) == true,
   * then *get() is copy-assigned to *src.get().
   * If static_cast<bool>(*this) == true and static_cast<bool>(src) == false,
   * then *get() is destroyed.
   * If static_cast<bool>(*this) == false and static_cast<bool>(src) == true,
   * then *get() is copy-constructed from *src.get().
   *
   * @post static_cast<bool>(*this) == static_cast<bool>(src) &&
   * (get() == nullptr) == (src.get() == nullptr).
   * @param src Assignment source.
   * @return *this.
   * @throw - Whatever T::T(const T&) and T::operator=(const T&) throw.
   */
  delayed_init& operator=(const delayed_init& src)
    noexcept(
      std::is_nothrow_copy_constructible<T>::value &&
      std::is_nothrow_copy_assignable<T>::value
    ) {
    assign(static_cast<bool>(src), *src.get());
    return *this;
  }
  
  /**
   * @brief Copy-assignment to delayed_init<U>.
   *
   * If static_cast<bool>(*this) == true and static_cast<bool>(src) == true,
   * then *get() is copy-assigned to *src.get().
   * If static_cast<bool>(*this) == true and static_cast<bool>(src) == false,
   * then *get() is destroyed.
   * If static_cast<bool>(*this) == false and static_cast<bool>(src) == true,
   * then *get() is copy-constructed from *src.get().
   *
   * @post static_cast<bool>(*this) == static_cast<bool>(src) &&
   * (get() == nullptr) == (src.get() == nullptr).
   * @param src Assignment source.
   * @return *this.
   * @throw - Whatever T::T(const U&) and T::operator=(const U&) throw.
   */
  template <typename U>
  delayed_init& operator=(const delayed_init<U>& src)
    noexcept(
      std::is_nothrow_constructible<T, const U&>::value &&
      std::is_nothrow_assignable<T, const U&>::value
    ) {
    assign(static_cast<bool>(src), *src.get());
    return *this;
  }

  /**
   * @brief Move-assignment to delayed_init<U>.
   *
   * If static_cast<bool>(*this) == true and static_cast<bool>(src) == true,
   * then *get() is copy- or move-assigned to *src.get().
   * If static_cast<bool>(*this) == true and static_cast<bool>(src) == false,
   * then *get() is destroyed.
   * If static_cast<bool>(*this) == false and static_cast<bool>(src) == true,
   * then *get() is copy- or move-constructed from *src.get().
   *
   * @post static_cast<bool>(*this) == static_cast<bool>(src) &&
   * (get() == nullptr) == (src.get() == nullptr).
   * @param src Assignment source.
   * @return *this.
   * @throw - Whatever T::T(U&&) and T::operator=(U&&) throw.
   */
  template <typename U>
  delayed_init& operator=(delayed_init<U>&& src)
    noexcept(
      std::is_nothrow_constructible<T, U&&>::value &&
      std::is_nothrow_assignable<T, U&&>::value
    ) {
    assign(static_cast<bool>(src), std::move(*src.get()));
    return *this;
  }

  /**
   * @brief Assignment to U.
   *
   * If static_cast<bool>(*this) == true, then *get() is copy- or move-assigned
   * to obj. Othwerwise, *get() is copy- or move-constructed from obj.
   *
   * @pre std::is_convertible<U, T>::value == true.
   * @post static_cast<bool>(*this) == static_cast<bool>(src) &&
   * (get() == nullptr) == (src.get() == nullptr).
   * @param obj Assignment source.
   * @return *this.
   * @throw - Whatever T::T(U&&) and T::operator=(U&&) throw.
   */
  template <typename U, typename = typename
    std::enable_if<std::is_convertible<U, T>::value>::type>
  delayed_init& operator=(U&& obj)
    noexcept(
      std::is_nothrow_constructible<T, U&&>::value &&
      std::is_nothrow_assignable<T, U&&>::value
    ) {
    assign(true, std::forward<U>(obj_));
    return *this;
  }
  
  /**
   * @brief Indirection.
   *
   * @pre static_cast<bool>(*this) == true.
   * @return *get().
   * @throw std::logic_error If pre-condition doesn't hold.
   */
  T& operator*() {
    if (is_init_)
      return obj_;
    throw std::logic_error("attempt to use uninitialised object");
  } 

  /**
   * @brief Indirection (const).
   *
   * @pre static_cast<bool>(*this) == true.
   * @return *get().
   * @throw std::logic_error If pre-condition doesn't hold.
   */
  const T& operator*() const {
    return *const_cast<delayed_init*>(this);
  } 

  /**
   * @brief Getter.
   *
   * @return A pointer to the inner object if static_cast<bool>(*this) == true.
   * Otherwise, nullptr.
   * @throw - Nothing.
   */
  T* get() noexcept {
    return is_init_ ? &obj_ : nullptr;
  }
  
  /**
   * @brief Getter (const).
   *
   * @return A pointer to the inner object if static_cast<bool>(*this) == true.
   * Otherwise nullptr.
   * @throw - Nothing.
   */
  const T* get() const noexcept {
    return is_init_ ? &obj_ : nullptr;
  }
  
  /**
   * @brief Deference.
   *
   * @return get().
   * @throw - Nothing.
   */
  T* operator->() noexcept {
    return get();
  }

  /**
   * @brief Deference (const).
   *
   * @return get().
   * @throw - Nothing.
   */
  const T* operator->() const noexcept {
    return get();
  }

  /**
   * @brief Conversion to bool.
   *
   * @return false if the inner object was not initialised. Otherwise, true.
   * @throw - Nothing.
   */
  explicit operator bool() const noexcept {
    return is_init_;
  }

  /**
   * @brief Initialiser.
   *
   * Builds inner object by forwarding arguments to T's constructor.
   *
   * @pre static_cast<bool>(*this) == false.
   * @post static_cast<bool>(*this) == true && get() != nullptr.
   * @param args Initialisation arguments.
   * @throw - std::logic_error (if pre condition doesn't hold) and whatever
   * T::T(Args&&...) throws.
   */
  template <typename... Args>
  void init(Args&&... args) {
    if (is_init_)
      throw std::logic_error("second attempt to initialise object");
    init_obj(std::forward<Args>(args)...);
  }
  
  /**
   * @brief Swap.
   *
   * If static_cast<bool>(*this) == true and static_cast<bool>(src) == true,
   * then *get() and *src.get() are swapped.
   * If static_cast<bool>(*this) == true and static_cast<bool>(src) == false,
   * then *get() is copied or moved to src.get() and then destroyed.
   * If static_cast<bool>(*this) == false and static_cast<bool>(src) == true,
   * then *src.get() is copied or moved to *get() and destroyed.
   *
   * @param src Source.
   * @throw - Whatever T::(const T&), T::(T&&) and swap(T&, T&) throw.
   */
  void swap(delayed_init& src)
    noexcept(
      std::is_nothrow_copy_constructible<T>::value &&
      std::is_nothrow_move_constructible<T>::value &&
      traits::is_nothrow_swappable<T>::value
    ) {
    if (is_init_) {
      if (src.is_init_) {
        
        using std::swap;
        swap(obj_, src.obj_);
      }
      else {
        src.init_obj(std::move(obj_));
        destroy();
      }
    }
    else if (src.is_init_) {
      init_obj(std::move(src.obj_));
      src.destroy();
    }
  }

private:

  bool is_init_ = false;
  union {
    T obj_;
  };

  /**
   * @brief Initialise inner object.
   *
   * @pre static_cast<bool>(*this) == false.
   * @post static_cast<bool>(*this) == true && get() != nullptr.
   * @param args Initialisation arguments.
   * @throw - Whatever T::T(Args&&...) throws.
   */
  template <typename... Args>
  void init_obj(Args&&... args) {
    new ((void *) &obj_) T(std::forward<Args>(args)...);
    is_init_ = true;
  }
  
  /**
   * @brief Initialise *this (from another delayed_init members).
   *
   * @pre static_cast<bool>(*this) == false.
   * @post static_cast<bool>(*this) == src_is_init &&
   * (get() != nullptr) == src_is_init.
   * @param src_is_init The source's is_init_ member.
   * @param src_obj The source's obj_ member.
   * @throw - Whatever init_obj(U&&) throws.
   */
  template <typename U>
  void init_me(bool src_is_init, U&& src_obj) {
    if (src_is_init)
      init_obj(std::forward<U>(src_obj));
  }
  
  /**
   * @brief assign *this (from another delayed_init members).
   *
   * @post static_cast<bool>(*this) == src_is_init &&
   * (get() != nullptr) == src_is_init.
   * @param src_is_init The source's is_init_ member.
   * @param src_obj The source's obj_ member.
   * @throw - Whatever init_me(bool, U&&) and T::operator =(U&&) throw.
   */
  template <typename U>
  void assign(bool src_is_init, U&& src_obj) {
    if (!is_init_)
      init_me(src_is_init, std::forward<U>(src_obj));
    else if (src_is_init)
      obj_ = std::forward<U>(src_obj);
    else
      destroy();
  }

  /**
   * @brief Destroy inner object.
   *
   * T:~T() must not throw.
   *
   * @post static_cast<bool>(*this) == false && get() == nullptr
   * @throw - Nothing.
   */
  void destroy() noexcept {
    if (is_init_)
      (&obj_)->~T();
    is_init_ = false;
  }

}; // class delayed_init

/**
 * @brief Swap two delayed_init objects.
 *
 * @param d1 1st delayed_init object.
 * @param d2 2nd delayed_init object.
 *
 * @throw - Whatever T::swap(T&) throws.
 */
template <typename T>
void swap(delayed_init<T>& d1, delayed_init<T>& d2)
  noexcept(noexcept(std::declval<T&>().swap(std::declval<T&>()))) {
  d1.swap(d2);
}

} // namespace overload

#endif // OVERLOAD_DELAYED_INIT_H_
