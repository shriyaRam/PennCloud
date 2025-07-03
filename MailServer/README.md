# Mail Server

This is the mail server of the project. 

## Prerequisites

You need to first install gRPC and CMake. Check the demo repository shared by the TA for detailed instructions.

## How to Run the Server

If you are in the root directory:

1. **Create a build folder** (for the first time only)

   ```bash
   cd MailServer
   mkdir build
   cd build

2. **Compile the files**

   ```bash
    cmake ..
    make
    
3. **Run the server (from inside build/server)**

   ```bash

    ./server/smtp_server ../server_config.txt server_index

    Replace server_index with a value from 0 to 3.

