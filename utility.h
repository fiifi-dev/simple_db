//
// Created by fii on 2/2/24.
//
// Include standard libraries for various functionalities
#include <cassert>      // For assert function, used to handle internal errors
#include <cstdint>      // For fixed-width integer types
#include <cstdlib>      // For standard library functions like malloc
#include <cstring>      // For string manipulation functions
#include <cstdio>       // For standard, I/O functions
#include <cerrno>       // For error number definitions
#include <fcntl.h>      // For file control options
#include <poll.h>       // For the poll function, used in I/O multiplexing
#include <unistd.h>     // For POSIX API, like read/write/close
#include <arpa/inet.h>  // For network byte order conversions
#include <sys/socket.h> // For socket API functions
#include <netinet/ip.h> // For IP protocol definitions
#include <string>       // For std::string class
#include <vector>       // For std::vector container
#include <map>          // For std::map container

#include "types.h"

#ifndef FII_DB_UTILITY_H
#define FII_DB_UTILITY_H

void msg(const char *msg);

void die(const char *msg);

void fd_set_nb(int fd);

void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn);

int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd);

void state_req(Conn *conn);

void state_res(Conn *conn);

int32_t parse_req(
    const uint8_t *data,
    size_t len,
    std::vector<std::string> &out);

uint32_t do_get(
    const std::vector<std::string> &cmd,
    uint8_t *res,
    uint32_t *reslen);

uint32_t do_set(
    const std::vector<std::string> &cmd,
    uint8_t *res,
    uint32_t *reslen);

uint32_t do_del(
    const std::vector<std::string> &cmd,
    uint8_t *res,
    uint32_t *reslen);

bool cmd_is(const std::string &word, const char *cmd);

int32_t do_request(
    const uint8_t *req,
    uint32_t reqlen,
    uint32_t *rescode,
    uint8_t *res,
    uint32_t *reslen);

bool try_one_request(Conn *conn);

bool try_fill_buffer(Conn *conn);

bool try_flush_buffer(Conn *conn);

void connection_io(Conn *conn);

int32_t read_full(int fd, char *buf, size_t n);

int32_t write_all(int fd, const char *buf, size_t n);

int32_t send_req(int fd, const std::vector<std::string> &cmd);

int32_t read_res(int fd);

#endif // FII_DB_UTILITY_H
