# Chops Net IP FAQ

- Why should I use Chops Net IP?
  - If you need general IP (Internet Protocol) networking in your application it will make your development job easier.
- Why not use one of the many other networking or socket libraries?
  - Chops Net IP scales very well, using system resources efficiently. It performs well. It is portable across many compilers and platforms. It defines application customization points that few other libraries have. The abstractions it provides are flexible and useful in many domains. It provides TCP, UDP, and UDP multicasting functionality, abstracting away many of the differences between those protocols.
- When should I not use Chops Net IP?
  - When you need abstractions for a specific complex and widely used domain. For example, serious web server applications should consider using Boost Beast (written by Vinnie Falco) or one of the many web frameworks that are available. If absolute raw performance is needed, writing directly to the Asio API (or similar) would be better, giving up the convenience and flexibility that Chops Net IP provides.
- Is Chops Net IP a framework?
  - No. It is a general purpose library. There are no specific network protocols required by the library and no wire protocols added by the library. It can communicate with any TCP or UDP application. Obviously the wire protocols and communication semantics need to be appropriately implemented by the application using the Chops Net IP library.
- Wny is a queue required for outgoing data?
  - Applications may send data faster than it can be consumed at the remote end (or passed through the local network stack). Queueing the outgoing data allows timely processing if this situation occurs. There is an API design choice for outgoing data: 1) notify the application when outgoing data can be sent 2) queue the data when needed and let the Chops Net IP library manage the outgoing data notifications. The Chops Net IP API philosophy is "fire and forget" (i.e. all calls immediately return to the application), which leads to an outgoing queue. However, applications may have different memory constraints, and the Chops Net IP library allows different containers to be used (at compile) time for the outgoing queue. For example, a fixed size ring buffer or circular queue can be used (as long as the queue requirements are met).
- What if the outgoing data queue becomes large?
  - This is an indication that the remote end is not processing data fast enough (or that data is being produced too fast). The application can query outgoing queue stats to determine if this scenario is occurring.
- Why not provide a configuration API for a table-driven network application?
  - There are many different formats and types of configurations, including dynamically generated configurations. Configuration should be a separate concern from the Chops Net IP library. Configuration parsing for common formats (e.g. JSON) may be added to the `component` directory (non-dependent convenience classes and functions) in the future or provided in separate repositories.
- Are there logging or tracing facilities in Chops Net IP?
  - No. There is not a logging library or design that is generic enough to work in every environment. Even general purpose logging libraries have a set of tradeoffs that are not acceptable for Chops Net IP. Instead, the approach taken by Chops Net IP is to provide customization points for every meaningful step in the networking processing flow. This allows the application complete flexibility in using whatever logging facilities it wishes.
- Is Chops Net IP a complete wrapper over the Asio API?
  - No. There are access points that expose various Asio facilities, and Asio `endpoints` are used in the Chops Net IP API. In particular, Chops Net IP provides an interface to access the underlying Asio socket and the application can directly set (or query) socket options (versus wrapping and exactly duplicating the functionality).
- What are some of the subtle design challenges in the implementation?
  - Passing application supplied function objects through the library layers is central to the design. Since these are passed across thread boundaries at certain points, knowing when to call `std::move` versus `std::forward<F>` is crucial (and has been the source of more than one bug).
  - The shutdown notification logic is tricky and hard to get correct, specially between the TCP connection object and the TCP acceptor and TCP connector objects.
  - `std::atomic` variables can be used for critical section locks, but care must be taken, and the Chops Net IP implementation uses the `compare_exchange_strong` method on a bool atomic to guard against multiple threads calling `start` or `stop` at the same time.

