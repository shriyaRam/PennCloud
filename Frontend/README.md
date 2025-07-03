# Frontend Server

This is the frontend server of the project. It listens for HTTP requests from the browser and returns HTTP responses for webpage rendering.

## Prerequisites

You need to first install gRPC and CMake. Check the demo repository shared by the TA for detailed instructions.

## How to Run the Server

If you are in the root directory:

1. **Create a build folder** (for the first time only)

   ```bash
   cd Frontend
   mkdir build
   cd build

2. **Compile the files**

   ```bash
    cmake ..
    make
    
3. **Run the server**

   ```bash
    ./client/frontend ../client/src/server_config.txt server_index

    Replace server_index with a value from 1 to 4.

4. **Access the server**

Check the terminal for the port number, then visit localhost:port in your browser. You should see the login page.

