#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include <experimental/io_context>

#include <chrono>
#include <thread>

#include "timer/periodic_timer.hpp"
#include "utility/repeat.hpp"

template <typename Clock, typename Dur, typename TP>
void timer_test (int iterations, Dur timer_dur, Dur wait_time, TP start_time) {

  chops::periodic_timer<Clock> timer;
  std::experimental::net::io_context ioc;
  std::experimental::net::executor_work_guard<std::experimental::net::io_context::executor_type>
    wg { std::experimental::net::make_work_guard(ioc) };

  std::thread thr([&ioc] () { ioc.run(); } );
  std::this_thread::sleep_for(wait_time);
  wg.reset();
  thr.join();


  
}

TEST_CASE( "Testing periodic timer", "[periodic_timer]" ) {
  
  SECTION ( "Testing instantiation and basic method operation in non threaded operation" ) {
    chops::wait_queue<int> wq;
    non_threaded_int_test(wq);
  }

  SECTION ( "Testing ring_span instantiation and basic method operation in non threaded operation" ) {
    const int sz = 10;
    int buf[sz];
    chops::wait_queue<int, nonstd::ring_span<int> > wq(buf+0, buf+sz);
    non_threaded_int_test(wq);
  }
  
}

