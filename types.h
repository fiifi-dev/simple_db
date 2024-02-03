//
// Created by fii on 2/2/24.
//

// Include standard libraries for various functionalities
#include <string>  // For assert function, used to handle internal errors
#include <cstdint> // For fixed-width integer types
#include <cstdlib> // For standard library functions like malloc
#include <map>     // For std::map container

#ifndef FII_DB_TYPES_H
#define FII_DB_TYPES_H
// Define maximum message size for simplicity in handling buffers
const size_t k_max_msg = 4096;

// Maximum number of arguments in a command
const size_t k_max_args = 1024;

// Global map to store key-value pairs, acting as a simple database
static std::map<std::string, std::string> g_map;

enum CONNECTION_STATE
{
    STATE_REQ = 0, // State indicating waiting for a request
    STATE_RES = 1, // State indicating sending a response
    STATE_END = 2, // State indicating the connection should be closed
};

// Enumeration for response codes
enum RESPONSE_CODES
{
    RES_OK = 0,  // Indicates a successful operation
    RES_ERR = 1, // Indicates an error occurred
    RES_NX = 2,  // Indicates a non-existent item
};

// Structure representing a network connection
struct Conn
{
    int fd = -1;                 // File descriptor for the connection socket
    uint32_t state = 0;          // Current state of the connection (using the enum above)
    size_t rbuf_size = 0;        // Size of the data currently in the read buffer
    uint8_t rbuf[4 + k_max_msg]; // Read buffer, with space for message length and data
    size_t wbuf_size = 0;        // Size of the data currently in the write buffer
    size_t wbuf_sent = 0;        // Amount of data already sent from the write buffer
    uint8_t wbuf[4 + k_max_msg]; // Write buffer, with space for message length and data
};

#endif // FII_DB_TYPES_H
