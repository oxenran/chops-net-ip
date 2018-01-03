/** @file
 *
 *  @ingroup test_module
 *
 *  @brief Declarations for utility code shared between @c net_ip tests.
 *
 *  @author Cliff Green
 *  @date 2017, 2018
 *  @copyright Cliff Green, MIT License
 *
 */

#include <string_view>
#include <cstddef> // std::size_t
#include <vector>

#include <experimental/buffer>

#include "utility/shared_buffer.hpp"

#ifndef SHARED_UTILITY_TEST_HPP_INCLUDED
#define SHARED_UTILITY_TEST_HPP_INCLUDED

chops::mutable_shared_buffer make_body_buf(std::string_view pre, char body_char, std::size_t num_body_chars);
chops::mutable_shared_buffer make_variable_len_msg(const chops::mutable_shared_buffer& body);
chops::mutable_shared_buffer make_cr_lf_text_msg(const chops::mutable_shared_buffer& body);
chops::mutable_shared_buffer make_lf_text_msg(const chops::mutable_shared_buffer& body);

std::size_t variable_len_msg_frame(std::experimental::net::const_buffer);

using vec_buf = std::vector<chops::mutable_shared_buffer>;

template <typename F>
vec_buf make_msg_set(F&& f, std::string_view pre, char body_char, int num_msgs) {
  vec_buf vec;
  chops::repeat(num_msgs, [pre, body_char] (const int& i) {
      vec.push_back (f(make_body_buf(pre, body_char, i+1)));
    }
  );
  return vec;
}

bool compare_msg_sets(const vec_buf&, const vec_buf&);

#endif

