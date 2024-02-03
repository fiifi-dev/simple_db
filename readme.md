
# Simple DB

A Simple Database similar to redis: it allows get, set and delete operations

## Getting Started

These instructions will get your copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

Linux, CMake

### Installation

A step-by-step guide to get a development environment running.

1. **Create Output Folder**

    To create a build directory for the project, use the following commands:

    ```bash
    mkdir build
    cd build
    ```

2. **Build Applications**

    Compile the project using CMake and Make:

    ```bash
    cmake ..
    make
    ```

### Running the Applications

Here's how you can run the server and client applications.

#### Running the Server

To run the server application, use the following command:

```bash
./server
```

#### Running the Client

The client application supports various commands such as `get`, `set`, `del`, and `unk`. Below are some examples of using these commands:

- **Get a Key:**

    If the key does not exist, the server will respond with no results:

    ```bash
    ./client get key
    # server says: [2] // no results
    ```

- **Set a Key:**

    Store a value with a key:

    ```bash
    ./client set key val
    # server says: [0]
    ```

- **Retrieve a Key:**

    Retrieve the value for a key:

    ```bash
    ./client get key
    # server says: [0] val
    ```

- **Delete a Key:**

    Delete a key-value pair:

    ```bash
    ./client del key
    # server says: [0]
    ```

    Subsequent attempts to get the deleted key will show that it's been deleted:

    ```bash
    ./client get key
    # server says: [2] // it's been deleted
    ```

- **Unknown Command:**

    Using an unknown command will prompt an error from the server:

    ```bash
    ./client unk val
    # server says: [1] Unknown cmd
    ```

