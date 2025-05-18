---
title: Socket Programming
markmap:
  colorFreezeLevel: 2
  maxWidth: 450
---

## Server File Descriptor

- A file descriptor is an integer that uniquely identifies an open file or socket in a process.
- In the context of socket programming, a file descriptor is used to refer to a network socket once it has been created or opened, allowing the program to communicate with other processes over the network.
- `socket` function creates a server file descriptor

  - codes
    ````cpp
        int server_fd;
        // create a socket file descriptor
        server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (server_fd < 0)
        {
            perror("Socket Connection failed");
            exit(EXIT_FAILURE);
        }
        ```
    ````
  - args

    1. address family
       - `AF_INET` => IPv4
       - `AF_INET6` => IPv6
       - `AF_UNIX` => local inter-process communication
    2. socket type
       - `SOCK_STREAM` => TCP
       - `SOCK_DGRAM` => UDP
       - `SOCK_RAW` => raw network protocols
    3. protocol
       - | Protocol       | Value | Description                                        |
         | -------------- | ----- | -------------------------------------------------- |
         | `IPPROTO_IP`   | `0`   | Dummy protocol for TCP/UDP, system chooses default |
         | `IPPROTO_TCP`  | `6`   | Transmission Control Protocol (TCP)                |
         | `IPPROTO_UDP`  | `17`  | User Datagram Protocol (UDP)                       |
         | `IPPROTO_ICMP` | `1`   | Internet Control Message Protocol (used for ping)  |
         | `IPPROTO_RAW`  | `255` | Raw IP packets                                     |

## Socket Options

- It configure the socket behavior with parameters
- `setsockopt` function configures the socket behavior

  - codes
    ```cpp
    int opt = 1;
    // set socket options
    int socket_options_config = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (socket_options_config < 0)
    {
        perror("Socket options config failed.");
        exit(EXIT_FAILURE);
    }
    ```
  - args

    1. `socket`: The file descriptor for the socket (server_fd in this case).
    2. `level`: The protocol level where the option is defined. SOL_SOCKET is used for socket-level options.
       - `SOL_SOCKET` => Options that apply to the socket layer
       - `IPPROTO_TCP` => Options specific to TCP
       - `IPPROTO_IP` => Options specific to IP
       - `IPPROTO_IPV6` => Options specific to IPv6
       - `IPPROTO_UDP` => Options specific to the UDP protocol.
       - `SOL_RAW` => Options for raw sockets, which allow applications to directly handle lower-layer protocols.
    3. `option_name`: The name of the option to set.

       - Each protocol level set in the 2nd argument has its special options to set

         - `SOL_SOCKET`
           - | Option         | Type             | Description                                      |
             | -------------- | ---------------- | ------------------------------------------------ |
             | `SO_REUSEADDR` | `int`            | Reuse local address                              |
             | `SO_REUSEPORT` | `int`            | Reuse local port (maybe Mac os does not support) |
             | `SO_KEEPALIVE` | `int`            | Keep TCP connection alive                        |
             | `SO_RCVBUF`    | `int`            | Set receive buffer size                          |
             | `SO_SNDBUF`    | `int`            | Set send buffer size                             |
             | `SO_RCVTIMEO`  | `struct timeval` | Set receive timeout                              |
             | `SO_SNDTIMEO`  | `struct timeval` | Set send timeout                                 |
         - `IPPROTO_IP`

           - | Option               | Type             | Description                     |
             | -------------------- | ---------------- | ------------------------------- |
             | `IP_TTL`             | `int`            | Set default time-to-live        |
             | `IP_HDRINCL`         | `int`            | Include IP header (raw sockets) |
             | `IP_OPTIONS`         | buffer           | Set IP options                  |
             | `IP_MULTICAST_TTL`   | `u_char`         | TTL for multicast packets       |
             | `IP_MULTICAST_LOOP`  | `u_char`         | Enable multicast loopback       |
             | `IP_ADD_MEMBERSHIP`  | `struct ip_mreq` | Join multicast group            |
             | `IP_DROP_MEMBERSHIP` | `struct ip_mreq` | Leave multicast group           |

         - `IPPROTO_IPV6`

           - | Option                | Type               | Description                   |
             | --------------------- | ------------------ | ----------------------------- |
             | `IPV6_UNICAST_HOPS`   | `int`              | Hop limit for unicast packets |
             | `IPV6_MULTICAST_HOPS` | `int`              | Hop limit for multicast       |
             | `IPV6_MULTICAST_LOOP` | `int`              | Enable loopback for multicast |
             | `IPV6_JOIN_GROUP`     | `struct ipv6_mreq` | Join IPv6 multicast group     |
             | `IPV6_LEAVE_GROUP`    | `struct ipv6_mreq` | Leave IPv6 multicast group    |
             | `IPV6_V6ONLY`         | `int`              | Restrict socket to IPv6 only  |

         - `IPPROTO_TCP`

           - | Option          | Type  | Description                                                |
             | --------------- | ----- | ---------------------------------------------------------- |
             | `TCP_NODELAY`   | `int` | Disable Nagle's algorithm (send small packets immediately) |
             | `TCP_MAXSEG`    | `int` | Set maximum segment size                                   |
             | `TCP_KEEPIDLE`  | `int` | Idle time before keepalive probes (Linux only)             |
             | `TCP_KEEPINTVL` | `int` | Interval between keepalive probes                          |
             | `TCP_KEEPCNT`   | `int` | Max number of keepalive probes                             |

         - `IPPROTO_UDP`

           - | Option             | Type  | Description                                                  |
             | ------------------ | ----- | ------------------------------------------------------------ |
             | `UDP_CORK`         | `int` | Used to control packet aggregation in Linux.                 |
             | `UDP_NO_CHECK6_TX` | `int` | Disable checksum generation for outgoing IPv6 UDP packets    |
             | `UDP_NO_CHECK6_RX` | `int` | Disable checksum verification for incoming IPv6 UDP packets. |

         - `SOL_RAW`
           - | Option        | Type  | Description                                                     |
             | ------------- | ----- | --------------------------------------------------------------- |
             | `RAW_FILTER`  | `int` | Set a filter for incoming packets (Linux-specific).             |
             | `RAW_HDRINCL` | `int` | Include the protocol header in packets sent via the raw socket. |

       - You can pass multiple option names by using `bitwise OR operator

         - It is used to combine multiple flags (or options) into a single value by performing a bitwise OR operation on their binary representations.
         - `SO_REUSEADDR | SO_REUSEPORT`

           - SO_REUSEADDR(0x0004) | SO_REUSEPORT(0x0200) = 0x0204

             - ```ymal
                    0x0004  = 0000 0000 0000 0100
                OR  0x0200  = 0000 0010 0000 0000
                -------------------------------
                result      = 0000 0010 0000 0100 = 0x0204

               ```

       - But you should call `setsockopt()` separately for each option since some option value type can differ

    4. `option_value`: A pointer to the value for the option. Here, &opt is passed.
    5. `option_len`: The size of the option_value. Here, sizeof(opt)
