# Ping CLI Application

## What is it?

It is a small Ping CLI application for MacOS or Linux supporting both IPv4 and IPv6. 

## Install

```sh
$ cd  your/project/root/directory
$ make
```

## Usage

You can add some arguments when starts the program as following.

| argument      | meaning                                                      |
| ------------- | ------------------------------------------------------------ |
| -4            | Use ipv4 protocol                                            |
| -6            | Use ipv6 protocol                                            |
| -c *count*    | The program will send *count* ICMP messages in total.        |
| -t *max_ttl*  | Set TTL as an argument, the program will report the corresponding "time exceeded” ICMP messages. |
| -i *interval* | Set *interval* seconds between two sending ICMP attemp.      |

You can use it in the command line as following.

```sh
$ cd  your/project/root/directory
$ sudo ./main [-4] [-6] [-c count] [-t max ttl] [-i interval] [hostname/IP address]
```

## Maintainer

Yudai Chen 

Email: yc121@rice.edu
