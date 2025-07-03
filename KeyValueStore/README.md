

Dependencies
To set up the project and install the necessary dependencies, follow these steps:

Step 1: Install Dependencies
Run the following command to install all the necessary dependencies:

bash dependencies.sh  # Do this only once
This script will ensure that all required libraries, such as gRPC, protobuf, and others, are installed on your system.

Running the Key-Value Store Server
After setting up the dependencies, follow these steps to build and run the server.

Step 2: Build the Server
Create a build directory:
mkdir build

Change to the build directory:
cd build

Run cmake to configure the build:
cmake ..
This step configures the project and generates the necessary build files.

Compile the project:

make -j$(nproc)
The -j$(nproc) flag tells make to use all available CPU cores to speed up the build process.

Step 3: Run the Server
After the build is complete, you can run the server by following these steps:

Navigate to the build directory (if you're not already there):
cd build

Run the server with the configuration file and server index:
./server/Server ../../server_config.txt server_index
../../server_config.txt: This file contains the server configuration, including address-port pairs for each server. 
server_index: The index of the server you want to run (e.g., "1", "2", etc., depending on your configuration). This allows you to run multiple instances of the server based on your setup.


To test server, you can run the client
Navigate to the build directory (if you're not already there)
cd client
./Client

currently the server address is set as "localhost:50052" for the client, so it will send request to this server only
change this to send to other servers
