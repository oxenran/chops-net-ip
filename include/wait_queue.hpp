/** @file
 *
 *  @ingroup utility_module
 *
 *  @brief Multi-reader multi-writer wait queue class for transferring
 *  data between threads.
 *
 *  This utility class allows transferring data between threads with queue 
 *  semantics, using C++ std library general facilities (mutex, condition 
 *  variable). An internal container with queue semantics is managed within 
 *  this object. One of the template parameters is the container type, 
 *  allowing customization for specific use cases (see below for additional
 *  details).
 *
 *  Multiple writer and reader threads can access this object, although when 
 *  a value is pushed, only one reader thread will be notified to consume a 
 *  value.
 *
 *  If the @c close method is called, any reader threads calling @c wait_and_pop 
 *  are notified, and an empty value returned to those threads. Subsequent calls 
 *  to @c push will return a @c false value.
 *
 *  This class is based on code from the book Concurrency in Action by Anthony 
 *  Williams. The core logic in this class is the same as provided by Anthony 
 *  in his book, but the interfaces have changed and additional features added. 
 *  The name of the utility class template in Anthony's book is @c threadsafe_queue.
 *
 *  @note A fixed size buffer can be used for this utility, which eliminates
 *  queue memory management happening during a push or pop. In particular,
 *  the proposed @c std::ring_span container (C++ 20, most likely) works well 
 *  for this use case, and this code has been tested with @c ring-span lite from 
 *  Martin Moene. 
 *
 *  @note Iterators are not supported, due to obvious difficulties with maintaining 
 *  consistency and integrity. The @c apply method can be used to access the internal
 *  data in a threadsafe manner.
 *
 *  @note Copy and move construction or assignment for the whole queue is
 *  disallowed, since the use cases and underlying implications are not clear 
 *  for those operations. In particular, the exception implications for 
 *  assigning the internal data from one queue to another is messy, and the general 
 *  semantics of what it means is not clearly defined. If there is data in one 
 *  @c wait_queue that must be copied or moved to another, the @c apply method can 
 *  be used, (even if it is not as efficient as an internal copy or move).
 *
 *  @authors Cliff Green, Anthony Williams
 *  @date 2017
 *
 */

#ifndef WAIT_QUEUE_INCLUDED_H
#define WAIT_QUEUE_INCLUDED_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <utility> // std::move
#include <type_traits> // for noexcept specs

namespace chops {

template <typename T, typename Container = std::deque<T> >
class wait_queue {
private:
  mutable std::mutex m_mut;
  Container m_data_queue;
  std::condition_variable m_data_cond;
  bool m_closed = false;

  using lock_guard = std::lock_guard<std::mutex>;

public:

  using size_type = typename Container::size_type;

  wait_queue() = default;

  template <typename Iter>
  wait_queue(Iter beg, Iter end) : m_mut(), m_data_queue(beg, end), 
    m_data_cond(), m_closed(false) { }

  // disallow copy or move construction of the entire object
  wait_queue(const wait_queue&) = delete;
  wait_queue(wait_queue&&) = delete;

  // disallow copy or move assigment of the entire object
  wait_queue& operator=(const wait_queue&) = delete;
  wait_queue& operator=(wait_queue&&) = delete;

  // modifying methods
  /**
   * Open a previously closed @c wait_queue for processing. The initial state of 
   * a @c wait_queue is open.
   */
  void open() noexcept {
    lock_guard lk(m_mut);
    m_closed = false;
  }

  /**
   * Close a @c wait_queue for processing. All waiting reader threaders will be 
   * notified. Subsequent @c push operations will return @c false.
   */
  void close() noexcept {
    lock_guard lk(m_mut);
    m_closed = true;
    m_data_cond.notify_all();
  }

  /**
   * Push a value, by copying, to the @c wait_queue. A waiting reader thread (if any) 
   * will be notified that a value has been added.
   *
   * @param val Val to copy into the queue.
   *
   * @return @c true if successful, @c false if the @c wait_queue is closed.
   */
  bool push(const T& val) noexcept(std::is_nothrow_copy_assignable<T>::value) {
    lock_guard lk(m_mut);
    if (m_closed) {
      return false;
    }
    m_data_queue.push_back(val);
    m_data_cond.notify_one();
    return true;
  }

  /**
   * This method has the same semantics as the other @c push, except that the value will 
   * be moved (if possible) instead of copied.
   */
  bool push(T&& val) noexcept(std::is_nothrow_move_assignable<T>::value) {
    lock_guard lk(m_mut);
    if (m_closed) {
      return false;
    }
    m_data_queue.push_back(std::move(val));
    m_data_cond.notify_one();
    return true;
  }

  /**
   * Pop and return a value from the @c wait_queue as soon as an element is available, blocking 
   * and waiting on a writer thread if necessary.
   *
   * @return A value from the @c wait_queue, if the @c std::optional is not empty. If the 
   * @c std::optional is empty, the @c wait_queue has been closed.
   */
  std::optional<T> wait_and_pop() noexcept(true) { // note - add stuff to noexcept
    std::unique_lock<std::mutex> lk(m_mut);
    m_data_cond.wait ( lk, [this] { return m_closed || !m_data_queue.empty(); } );
    std::optional<T> val{};
    if (!m_data_queue.empty()) {
      val = std::move(m_data_queue.front());
      m_data_queue.pop_front();
    }
    return val;
  }

  /**
   * Pop and return a value from the @c wait_queue if an element is immediately 
   * available, otherwise return an empty @c std::optional.
   *
   * @return A value from the @c wait_queue, if the @c std::optional is not empty, otherwise 
   * the @c wait_queue is empty.
   */
  std::optional<T> try_pop() noexcept(true) {
    lock_guard lk(m_mut);
    if (m_data_queue.empty()) {
      return std::optional<T>{};
    }
    std::optional<T> val = std::move(m_data_queue.front());
    m_data_queue.pop_front();
    return val;
  }

  // non-modifying methods

  /**
   * Apply a non-modifying function object to all elements of the queue.
   *
   * The function object is not allowed to modify any of the elements. 
   * Is is passed a const reference to the element type.
   *
   * This method can be used when an iteration of the elements is needed,
   * such as to print the elements, or copy them to another container, or 
   * to interrogate values of the elements.
   */
  template <typename F>
  void apply(F&& f) const noexcept {
    for (const T& elem : m_data_queue) {
      f(elem);
    }
  }

  /**
   * Close a @c wait_queue for processing. All waiting reader threaders will be 
   * notified. Subsequent @c push operations will return @c false.
   *
   * @return @c true if the @c wait_queue has been closed.
   */
  bool is_closed() const noexcept {
    lock_guard lk(m_mut);
    return m_closed;
  }

  /**
   * Query whether the @c wait_queue is empty or not.
   *
   * @return @c true if the @c wait_queue is empty.
   */
  bool empty() const noexcept {
    lock_guard lk(m_mut);
    return m_data_queue.empty();
  }

  /**
   * Get the number of elements in the @c wait_queue.
   *
   * @return Number of elements in the @c wait_queue.
   */
  size_type size() const noexcept {
    lock_guard lk(m_mut);
    return m_data_queue.size();
  }

};

} // end namespace

#endif

