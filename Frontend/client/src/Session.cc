#include "Session.h"

// Constructor

Session::Session() {
    this->username = "";
    this->session_id = "";
    this->last_access_time = std::time(nullptr);
}

std::string& Session::get_address_port(){
    return address_port;
}

void Session::set_address_port(std::string address_port){
    this->address_port = address_port;
}

Session::Session(std::string username) {
    this->username = username;
    this->session_id = generate_session_id();
    this->last_access_time = std::time(nullptr);
    this->is_valid = true;
}

bool Session::get_valid(){
    return is_valid;
}

void Session::set_valid(bool valid){
    is_valid = valid;
}

void Session::set_username(std::string username){
    this->username = username;
}

void Session::set_session_id(std::string session_id){
    this->session_id = session_id;
}

// Getters
std::string Session::get_username() {
    return username;
}

std::string Session::get_session_id() {
    update_last_access_time();
    return session_id;
}

std::time_t Session::get_last_access_time() {
    return last_access_time;
}

// Update last access time
void Session::update_last_access_time() {
    last_access_time = std::time(nullptr);
}

// Check if the session is expired
bool Session::is_expired() {
    return std::time(nullptr) - last_access_time > SESSION_TIMEOUT;
}

// Generate a random session ID
std::string Session::generate_session_id() {
    
    // random number generator
    std::random_device rd; 
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 35); // Values from 0-9 and a-z

    const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::string session_id;
    for (int i = 0; i < 32; ++i) { // Generate 32-character session_id
        session_id += chars[dis(gen)];
    }
    return session_id;
}

Session& Session::operator=(const Session &rhs){
    this->session_id = rhs.session_id;
    this->username = rhs.username;
    this->last_access_time = rhs.last_access_time;
    this->is_valid = rhs.is_valid;
    this->address_port = rhs.address_port;
    return *this;
}

void Session::clear(){
    this->username = "";
    this->last_access_time = std::time(nullptr);
    this->is_valid = false;
}
