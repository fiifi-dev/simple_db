//
// Created by fii on 2/2/24.
//
#include "utility.h"
#include "types.h"

// Prints a message to standard error
void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg); // Use fprintf to print to stderr
}

// Prints an error message to standard error and aborts the program
void die(const char *msg)
{
    int err = errno;                        // Store the current errno value
    fprintf(stderr, "[%d] %s\n", err, msg); // Print the error number and message
    abort();                                // Terminate the program
}

// Sets a file descriptor to non-blocking mode
void fd_set_nb(int fd)
{
    errno = 0;                         // Reset errno
    int flags = fcntl(fd, F_GETFL, 0); // Get current flags of the file descriptor
    if (errno)
    {
        die("fcntl error"); // If error occurred in fcntl, terminate program
        return;
    }

    flags |= O_NONBLOCK; // Add non-blocking flag

    errno = 0;                       // Reset errno
    (void)fcntl(fd, F_SETFL, flags); // Set new flags
    if (errno)
    {
        die("fcntl error"); // If error occurred in fcntl, terminate program
    }
}

// Maps a file descriptor to its corresponding connection object
void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn)
{
    if (fd2conn.size() <= (size_t)conn->fd)
    {                                 // If the vector is too small
        fd2conn.resize(conn->fd + 1); // Resize the vector to fit the file descriptor
    }
    fd2conn[conn->fd] = conn; // Store the connection pointer in the vector
}

// Accepts a new connection, initializes a Conn struct for it, and stores it in fd2conn
int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd)
{
    struct sockaddr_in client_addr = {};                                // Client address structure
    socklen_t socklen = sizeof(client_addr);                            // Length of the client address structure
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen); // Accept new connection
    if (connfd < 0)
    {
        msg("accept() error"); // Print error message if accept fails
        return -1;             // Return error code
    }

    fd_set_nb(connfd); // Set the new connection to non-blocking mode

    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn)); // Allocate memory for new connection
    if (!conn)
    {                  // If malloc failed
        close(connfd); // Close the connection file descriptor
        return -1;     // Return error code
    }
    // Initialize the connection structure
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;

    conn_put(fd2conn, conn); // Store the connection in the map
    return 0;                // Return success code
}

// Parses a request from a client, extracting the arguments
int32_t parse_req(
    const uint8_t *data, size_t len, std::vector<std::string> &out)
{
    if (len < 4)
        return -1; // Return error if the data length is too short
    uint32_t n = 0;
    memcpy(&n, &data[0], 4); // Extract the number of arguments
    if (n > k_max_args)
        return -1; // Return error if too many arguments

    size_t pos = 4; // Position in the data, starting after the argument count
    while (n--)
    { // Loop over the arguments
        if (pos + 4 > len)
            return -1; // Return error if the data length is too short for argument size
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4); // Extract the argument size
        if (pos + 4 + sz > len)
            return -1;                                          // Return error if the data length is too short for the argument
        out.push_back(std::string((char *)&data[pos + 4], sz)); // Add the argument to the output vector
        pos += 4 + sz;                                          // Move to the next argument
    }

    if (pos != len)
        return -1; // Return error if there is leftover data
    return 0;      // Return success
}

// Handles 'get' command by retrieving the value for the given key
uint32_t do_get(
    const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    if (!g_map.count(cmd[1]))
        return RES_NX;                   // Return non-existent if key not found
    std::string &val = g_map[cmd[1]];    // Retrieve the value for the key
    assert(val.size() <= k_max_msg);     // Ensure the value size is within the maximum message size
    memcpy(res, val.data(), val.size()); // Copy the value to the response buffer
    *reslen = (uint32_t)val.size();      // Set the response length
    return RES_OK;                       // Return success code
}

// Handles 'set' command by storing the given key-value pair
uint32_t do_set(
    const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    (void)res;              // Unused parameter, avoid compiler warnings
    (void)reslen;           // Unused parameter, avoid compiler warnings
    g_map[cmd[1]] = cmd[2]; // Set the value for the key in the map
    return RES_OK;          // Return success code
}

// Handles 'del' command by removing the given key-value pair
uint32_t do_del(
    const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    (void)res;           // Unused parameter, avoid compiler warnings
    (void)reslen;        // Unused parameter, avoid compiler warnings
    g_map.erase(cmd[1]); // Remove the key from the map
    return RES_OK;       // Return success code
}

// Checks if a command matches a specified word
bool cmd_is(const std::string &word, const char *cmd)
{
    return 0 == strcasecmp(word.c_str(), cmd); // Compare the command and word case-insensitively
}

// Processes a client request and generates a response
int32_t do_request(
    const uint8_t *req, uint32_t reqlen,
    uint32_t *rescode, uint8_t *res, uint32_t *reslen)
{
    std::vector<std::string> cmd; // Vector to hold the parsed command
    if (0 != parse_req(req, reqlen, cmd))
    {
        msg("bad req"); // Print message if request parsing fails
        return -1;      // Return error code
    }
    if (cmd.size() == 2 && cmd_is(cmd[0], "get"))
    {
        *rescode = do_get(cmd, res, reslen); // Handle 'get' command
    }
    else if (cmd.size() == 3 && cmd_is(cmd[0], "set"))
    {
        *rescode = do_set(cmd, res, reslen); // Handle 'set' command
    }
    else if (cmd.size() == 2 && cmd_is(cmd[0], "del"))
    {
        *rescode = do_del(cmd, res, reslen); // Handle 'del' command
    }
    else
    {
        *rescode = RES_ERR; // Set error code for unrecognized command
        const char *msg = "Unknown cmd";
        strcpy((char *)res, msg); // Copy error message to response buffer
        *reslen = strlen(msg);    // Set response length
        return 0;                 // Return success
    }
    return 0; // Return success
}

// Attempts to parse and respond to a single request from the read buffer
bool try_one_request(Conn *conn)
{
    if (conn->rbuf_size < 4)
        return false; // Return false if not enough data to read request length
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4); // Read the length of the request
    if (len > k_max_msg)
    {
        msg("too long");         // Print message if request length exceeds maximum
        conn->state = STATE_END; // Set connection state to end
        return false;            // Return false
    }
    if (4 + len > conn->rbuf_size)
        return false; // Return false if not enough data for the entire request

    uint32_t rescode = 0; // Variable to store the response code
    uint32_t wlen = 0;    // Variable to store the length of the response
    int32_t err = do_request(
        &conn->rbuf[4], len,
        &rescode, &conn->wbuf[4 + 4], &wlen); // Process the request and generate a response
    if (err)
    {
        conn->state = STATE_END; // Set connection state to end if an error occurs
        return false;            // Return false
    }
    wlen += 4;                           // Add length of response code to total response length
    memcpy(&conn->wbuf[0], &wlen, 4);    // Copy total response length to write buffer
    memcpy(&conn->wbuf[4], &rescode, 4); // Copy response code to write buffer
    conn->wbuf_size = 4 + wlen;          // Set the size of the data in the write buffer

    size_t remain = conn->rbuf_size - 4 - len; // Calculate remaining data in read buffer
    if (remain)
    {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain); // Move remaining data to the beginning of the buffer
    }
    conn->rbuf_size = remain; // Update the size of the data in the read buffer

    conn->state = STATE_RES; // Change the connection state to response
    state_res(conn);         // Handle the response state

    return (conn->state == STATE_REQ); // Return true if the state is back to request
}

// Attempts to fill the read buffer with data from the connection
bool try_fill_buffer(Conn *conn)
{
    assert(conn->rbuf_size < sizeof(conn->rbuf)); // Ensure the read buffer is not full
    ssize_t rv = 0;                               // Variable to store the result of read
    do
    {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;      // Calculate available space in the buffer
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap); // Read data into the buffer
    } while (rv < 0 && errno == EINTR);                         // Retry if interrupted by a signal
    if (rv < 0 && errno == EAGAIN)
        return false; // Return false if no data is available
    if (rv < 0)
    {
        msg("read() error");     // Print message if read error occurs
        conn->state = STATE_END; // Set connection state to end
        return false;            // Return false
    }
    if (rv == 0)
    {
        msg("EOF");              // Print message if end of file is reached
        conn->state = STATE_END; // Set connection state to end
        return false;            // Return false
    }

    conn->rbuf_size += (size_t)rv;                 // Add the number of bytes read to the buffer size
    assert(conn->rbuf_size <= sizeof(conn->rbuf)); // Ensure the buffer is not overfilled

    while (try_one_request(conn))
    {
    }                                  // Process requests one by one
    return (conn->state == STATE_REQ); // Return true if the state is back to request
}

// Attempts to flush the write buffer to the connection
bool try_flush_buffer(Conn *conn)
{
    ssize_t rv = 0; // Variable to store the result of write
    do
    {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;          // Calculate remaining data to send
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain); // Write data to the connection
    } while (rv < 0 && errno == EINTR);                             // Retry if interrupted by a signal
    if (rv < 0 && errno == EAGAIN)
        return false; // Return false if the operation would block
    if (rv < 0)
    {
        msg("write() error");    // Print message if write error occurs
        conn->state = STATE_END; // Set connection state to end
        return false;            // Return false
    }
    conn->wbuf_sent += (size_t)rv;              // Add the number of bytes written to the sent counter
    assert(conn->wbuf_sent <= conn->wbuf_size); // Ensure the sent counter does not exceed buffer size
    if (conn->wbuf_sent == conn->wbuf_size)
    {                            // If all data has been sent
        conn->state = STATE_REQ; // Change the state back to request
        conn->wbuf_sent = 0;     // Reset the sent counter
        conn->wbuf_size = 0;     // Reset the buffer size
        return false;            // Return false to stop flushing
    }
    return true; // Return true to continue flushing
}

// Handles the response state for a connection
void state_res(Conn *conn)
{
    while (try_flush_buffer(conn))
    {
    } // Keep flushing the buffer until complete
}

// Handles the request state for a connection
void state_req(Conn *conn)
{
    while (try_fill_buffer(conn))
    {
    } // Keep filling the buffer and processing requests
}

// Processes I/O operations for a connection based on its state
void connection_io(Conn *conn)
{
    if (conn->state == STATE_REQ)
    {
        state_req(conn); // Handle request state
    }
    else if (conn->state == STATE_RES)
    {
        state_res(conn); // Handle response state
    }
    else
    {
        assert(0); // Assert failure if in an unexpected state
    }
}

// Reads exactly 'n' bytes from a file descriptor into a buffer
int32_t read_full(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n); // Perform read operation
        if (rv <= 0)
        {
            return -1; // Return error on read failure or unexpected EOF
        }
        assert((size_t)rv <= n); // Assert that read value is valid
        n -= (size_t)rv;         // Decrease the number of bytes left to read
        buf += rv;               // Advance the buffer pointer
    }
    return 0; // Success
}

// Writes exactly 'n' bytes to a file descriptor from a buffer
int32_t write_all(int fd, const char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n); // Perform write operation
        if (rv <= 0)
        {
            return -1; // Return error on write failure
        }
        assert((size_t)rv <= n); // Assert that write value is valid
        n -= (size_t)rv;         // Decrease the number of bytes left to write
        buf += rv;               // Advance the buffer pointer
    }
    return 0; // Success
}

// Sends a request message over a socket
int32_t send_req(int fd, const std::vector<std::string> &cmd)
{
    uint32_t len = 4; // Start with 4 bytes for the count of command strings
    for (const std::string &s : cmd)
    {
        len += 4 + s.size(); // Calculate total length including each string's length
    }
    if (len > k_max_msg)
    {
        return -1; // Return error if length exceeds maximum
    }

    char wbuf[4 + k_max_msg];  // Write buffer
    memcpy(&wbuf[0], &len, 4); // Copy length to buffer (assuming little endian)
    uint32_t n = cmd.size();   // Get number of command strings
    memcpy(&wbuf[4], &n, 4);   // Copy command count to buffer
    size_t cur = 8;            // Current position in buffer
    for (const std::string &s : cmd)
    {
        uint32_t p = (uint32_t)s.size();            // Get size of string
        memcpy(&wbuf[cur], &p, 4);                  // Copy string length to buffer
        memcpy(&wbuf[cur + 4], s.data(), s.size()); // Copy string data to buffer
        cur += 4 + s.size();                        // Advance current position
    }
    return write_all(fd, wbuf, 4 + len); // Write the entire buffer to the socket
}

// Reads a response message from a socket
int32_t read_res(int fd)
{
    char rbuf[4 + k_max_msg + 1];         // Read buffer
    errno = 0;                            // Clear errno
    int32_t err = read_full(fd, rbuf, 4); // Read first 4 bytes (length)
    if (err)
    {
        if (errno == 0)
        {
            msg("EOF"); // EOF encountered
        }
        else
        {
            msg("read() error"); // Read error
        }
        return err; // Return the error
    }

    uint32_t len = 0;      // Length of the message
    memcpy(&len, rbuf, 4); // Copy length from buffer (assuming little endian)
    if (len > k_max_msg)
    {
        msg("too long"); // Length exceeds maximum
        return -1;       // Return error
    }

    err = read_full(fd, &rbuf[4], len); // Read the message body
    if (err)
    {
        msg("read() error"); // Read error
        return err;          // Return the error
    }

    uint32_t rescode = 0; // Response code
    if (len < 4)
    {
        msg("bad response"); // Invalid response length
        return -1;           // Return error
    }
    memcpy(&rescode, &rbuf[4], 4);                                  // Copy response code from buffer
    printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]); // Print the response
    return 0;                                                       // Success
}
