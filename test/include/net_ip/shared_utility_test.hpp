/** @file
 *
 *  @ingroup test_module
 *
 *  @brief Declarations and implementations for utility code shared between 
 *  @c net_ip tests.
 *
 *  The general Chops Net IP test strategy is to have message senders and message 
 *  receivers, with a flag specifying whether the receiver is to loop back the
 *  messages. For TCP it is independent of whether the sender or receiver is an 
 *  acceptor or connector, although most tests have the connector being a sender. In 
 *  the test routines, coordination is typically needed to know when a connection has 
 *  been made or sender / receiver is ready so that message flow can start. At the 
 *  higher layers, the Chops Net IP library facilities provide connection state
 *  change function object callbacks.
 *
 *  When the message flow is finished, an empty body message is sent to the receiver
 *  (and looped back if the reply flag is set), which signals an "end of message 
 *  flow" condition. The looped back empty message may not arrive back to the 
 *  sender since connections or handlers are in the process of being taken down.
 *
 *  @author Cliff Green
 *
 *  Copyright (c) 2017-2019 by Cliff Green
 *
 *  Distributed under the Boost Software License, Version 1.0. 
 *  (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef SHARED_UTILITY_TEST_HPP_INCLUDED
#define SHARED_UTILITY_TEST_HPP_INCLUDED

#include <string_view>
#include <cstddef> // std::size_t, std::byte
#include <cstdint> // std::uint16_t
#include <vector>
#include <utility> // std::forward, std::move
#include <atomic>
#include <memory> // std::shared_ptr
#include <thread>
#include <system_error>

#include <cassert>
#include <limits>

#include "asio/buffer.hpp"
#include "asio/ip/udp.hpp" // ip::udp::endpoint
#include "asio/ip/address.hpp" // make_address

#include "utility/repeat.hpp"
#include "utility/make_byte_array.hpp"
#include "utility/cast_ptr_to.hpp"

#include "marshall/extract_append.hpp"
#include "marshall/shared_buffer.hpp"

#include "net_ip/io_interface.hpp"
#include "net_ip/io_output.hpp"

namespace chops {
namespace test {

struct net_entity_mock {

  io_handler_mock_ptr  iop;
  std::thread          thr;
  
  net_entity_mock() : iop(std::make_shared<io_handler_mock>()) { }

  using socket_type = double;
  using endpoint_type = int;

  constexpr static double special_val = 42.0;
  double dummy = special_val;

  bool started = false;

  bool is_started() const { return started; }

  double& get_socket() { return dummy; }

  template <typename F1, typename F2>
  bool start(F1&& io_state_chg_func, F2&& err_func ) {
    if (started) {
      return false;
    }
    started = true;
    thr = std::thread([ this, ios = std::move(io_state_chg_func), ef = std::move(err_func)] () mutable {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ios(io_interface_mock(iop), 1, true);
        ef(io_interface_mock(iop), 
                 std::make_error_code(chops::net::net_ip_errc::message_handler_terminated));
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ios(io_interface_mock(iop), 0, false);
      }
    );
    return true;
  }

  bool stop() {
    if (!started) {
      return false;
    }
    started = false;
    join_thr();
    return true;
  }

  void join_thr() { thr.join(); }

};

} // end namespace test
} // end namespace chops

#endif

