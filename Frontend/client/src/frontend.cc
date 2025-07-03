#include "FrontendServer.h"
#include "BalancerClient.h"
#include <pthread.h>
#include <unistd.h> // For sleep

// Heartbeat task function
void* send_heartbeat_task(void* arg) {
    std::string server_id = *(std::string*)arg;
    BalancerClient balancer_client;

    while (true) {
        balancer_client.send_heartbeat(server_id, "alive");
        sleep(1); // Send heartbeat every 1 seconds
    }

    return nullptr;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: ./frontend <config_file> <line_number>\n");
        return 1;
    }

    
    FrontendServer server(argv);
    std::cout << "Server started..." << std::endl;

    // Start the heartbeat thread
    pthread_t heartbeat_thread;
    std::string server_id = server.get_server_id();
    if (pthread_create(&heartbeat_thread, nullptr, send_heartbeat_task, &server_id) != 0) {
        //fprintf(stderr, "Failed to create heartbeat thread\n");
        return 1;
    }

    server.handle_client();

    pthread_join(heartbeat_thread, nullptr);

    return 0;
}
