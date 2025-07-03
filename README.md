# PennCloud Project

# How to run the project

To run the PennCloud project, you need to enter each folder, build the files, and run each component in a terminal.

## Prerequisites

You need to first install gRPC and CMake. Check the demo repository shared by the TA panels for detailed instructions.

https://github.com/LangQin0422/gRPC-Demo-CMake

## Run each component

### Frontend Server

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

### Frontend Load Balancer

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

### KeyValueStore

1. **Create a build folder** (for the first time only)

   ```bash
   cd KeyValueStore
   mkdir build
   cd build

2. **Compile the files**

   ```bash
    cmake ..
    make
    
3. **Run the server**

   Navigate to the build directory (if you're not already there):
   cd build 

   ```bash
    ./server/Server ../server_config.txt server_index

    Replace server_index with a value from 1 to 9.

Note: If you want to clear the snapshots and log files
   
   cd build/server

   Run
   ./Server clear
       

### Coordinator

If you are in the root directory:

1. **Create a build folder** (for the first time only)

   ```bash
   cd Coordinator
   mkdir build
   cd build

2. **Compile the files**

   ```bash
    cmake ..
    make
    
3. **Run the server**

   ```bash
    ./src/Coordinator ../server_config.txt

### Email Server

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
    
3. **Run the Email server**

   ```bash
    ./server/smtp_server ../../server_config.txt server_index
    Replace server_index with a value from 0 to 3.

4. **Run the SMTP client**

    ```bash
    ./server/SMTPclient

5. **Run the Thunderbird**

    ```bash
    ./server/thunderbird ../../server_config.txt 1
    
**
   
