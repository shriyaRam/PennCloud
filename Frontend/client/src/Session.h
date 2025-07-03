#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <ctime>
#include <random>

class Session {

private:
    std::string username;
    std::string session_id;
    std::time_t last_access_time;
    bool is_valid = true;

    std::string address_port;

    const int SESSION_TIMEOUT = 300; // 5 minutes

public:
    Session(std::string username);
    Session();

    // Getters
    std::string get_username();
    std::string get_session_id();
    std::time_t get_last_access_time();
    std::string& get_address_port();

    // Utility functions
    void update_last_access_time(); // Update last access time to current time
    bool is_expired(); // Check if the session is expired
    bool get_valid(); // Check if the session is valid
    void set_valid(bool valid); // Set the session to valid or invalid

    // Set the address and port of the server
    void set_address_port(std::string address_port);
    void set_username(std::string username);
    void set_session_id(std::string session_id);

    // clear the session
    void clear();

    // Operator overloading
    Session& operator=(const Session &rhs);

private:
    // Helper function to generate session_id
    std::string generate_session_id();
};

#endif // SESSION_H
