sfqtester
=========

A program to create TCP connections on multiple ports. It's meant for testing of SFQ and SFQ-based AQMs (such as BLUE), where packets are put in a virtual queue based on a hash of sender IP, receiver IP and receiver port (so-called Bloom filters). To test this without having a bunch of hosts, it is easier to send to multiple receiver side ports.

The reason for SFQ to hash just the triplet (sender IP, receiver IP, receiver port) and not including sender port, is that a host may create multiple TCP connections to the same port. Read more about SFQ and AQMs in general in the litterature.

Note
----

This program is written for GNU/Linux. It uses `pthreads` and `epoll`, so it isn't very portable.

As this program spawns a lot of threads, make sure that
  * the upper limit of threads a process is allowed to spawn is higher than the number of connections you try to make
  * the upper limit of thread stack size is high enough to allow for many connections
  * the number of file descriptors a process is allowed to have is higher than at least double of the number of connections you try to make

To ensure these settings, fiddle around with `ulimit` and check in the `/proc` directory for what these are.
 
Compiling
---------

Type `make` in the project root directory.

**Note**: If you don't have `colorgcc` installed, just that in the `Makefile` to `gcc`. I really recommend `colorgcc` though, because it adds colors to your errors and warnings which is nice to filter line noise away.

Run as a server
---------------

To start the program in server mode, start it with
```
./sfqtester -q <the first local port> -c <the expected number of connections>
```

Run as a client
---------------

To start the program in client mode, start it with
```
./sfqtester -p <the first remote port> -c <number of connections> <remote host>
```

If you want to bind to a local port as well, for analysis purposes for example, use the `-q` option.
