# #!/bin/bash

# sudo apt update

# apt install -y build-essential autoconf libtool pkg-config libsystemd-dev

# apt install -y cmake

# cd ~
# git clone --recurse-submodules -b v1.67.1 --depth 1 --shallow-submodules https://github.com/grpc/grpc

# cd grpc
# mkdir -p cmake/build
# pushd cmake/build
# cmake -DgRPC_INSTALL=ON \
#       -DgRPC_BUILD_TESTS=OFF \
#       -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
#       ../..

# make -j$(nproc) install

#!/bin/bash

# Update the package list
sudo apt update

# Install required dependencies with sudo
sudo apt install -y build-essential autoconf libtool pkg-config libsystemd-dev cmake

# Hardcoded installation directory
MY_INSTALL_DIR="/usr/local"

# Check if the 'grpc' directory exists and handle it
if [ -d "$HOME/grpc" ]; then
    echo "Directory 'grpc' already exists. Removing it..."
    rm -rf "$HOME/grpc"
fi

# Clone the gRPC repository
git clone --recurse-submodules -b v1.67.1 --depth 1 --shallow-submodules https://github.com/grpc/grpc || {
    echo "Failed to clone gRPC repository."
    exit 1
}

# Navigate to the gRPC directory
cd grpc || exit

# Create the build directory
mkdir -p cmake/build
pushd cmake/build || exit

# Configure CMake with hardcoded install directory
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX="$MY_INSTALL_DIR" \
      ../..

# Build and install
sudo make -j$(nproc) install || {
    echo "Build or install failed."
    exit 1
}

# Return to the previous directory
popd

echo "gRPC installed successfully in $MY_INSTALL_DIR."
