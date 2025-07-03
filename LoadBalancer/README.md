# Frontend Load Balancer

It listens for HTTP requests (port 5010) from the browser and redirect to one of the active frontend nodes using round robin

## Prerequisites

You need to first install gRPC and CMake. Check the demo repository shared by the TA for detailed instructions.

## How to Run the Server

If you are in the root directory:

1. **Create a build folder** (for the first time only)

   ```bash
   cd LoadBalancer
   mkdir build
   cd build

2. **Compile the files**

   ```bash
    cmake ..
    make
    
3. **Run the server**

   ```bash
    ./server/balancer ../server/src/server_config.txt

