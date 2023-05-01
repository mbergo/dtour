# dtour

A simple TCP traffic redirector that forwards TCP traffic from a specified source IP and port to a specified destination IP and port.

## Requirements

This program requires the libevent library for efficient event handling. To install libevent, run the following command:

```
sudo apt-get install libevent-dev
```

## Compilation

Compile the program using the following command:

```
gcc dtour.c -o dtour -levent
```

This will create an executable named `dtour`.

## Usage

Run the program as follows:

```
./dtour -from IP:port -to IP:port
```

Replace `IP:port` with the respective source and destination IP addresses and ports.

**Note:** This code will only work on a single connection and will not handle multiple simultaneous connections or the entire traffic between two IPs. To handle multiple connections, you would need to create a proxy server and use asynchronous I/O to handle multiple connections concurrently.

