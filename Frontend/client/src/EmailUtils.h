// EmailUtils.h
#ifndef EMAILUTILS_H
#define EMAILUTILS_H

#include <string>

// Function declaration for decodeURIComponent
std::string decodeURIComponent(const std::string &encoded);
std::string generate_headers(const std::string &sender, const std::string &receiver, std::string subject);
bool send_email_chunks(std::string &content, const std::string &server_address, std::string from_email, std::string to_email);
bool send_email(std::string &from_email, std::string &to_email, std::string &subject, std::string &content, std::string &server_address);
bool forward_email_chunks(std::string &content, const std::string &server_address, std::string from_email, std::string to_email);
bool forward_email(std::string &from_email, std::string &to_email, std::string &subject, std::string &content, std::string &server_address);
std::string urlDecode(const std::string& encoded);

#endif // EMAILUTILS_H