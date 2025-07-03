#include "Client.h"
#include <grpcpp/grpcpp.h>
#include "proto/coordinator.grpc.pb.h"
#include "proto/coordinator.pb.h"
#include "proto/smtpClient.grpc.pb.h"
#include "proto/smtpClient.pb.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include "email.h"
#include <map>
#include <fstream>
#include <pthread.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/file.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <random>
#include "GRPCCalls.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using smtpClient::SendEmailRequest;
using smtpClient::SendEmailResponse;
using smtpClient::SMTPService;

#ifndef CHUNK_SIZE
constexpr size_t CHUNK_SIZE = 1024 * 1024 * 3.9; // 3.9 MB
#endif
// vector of strings
vector<string> recpTos;
string server_address;

string FrontendEmailData::sender;
string FrontendEmailData::recipient;
int32_t FrontendEmailData::num_chunks;
string FrontendEmailData::server_address_;
string FrontendEmailData::emailData;

// Declares verbose as extern so it can access the definition from smtp.cc
extern bool verbose;
// Global Email object with default values
Email globalEmail("", "", "");

// Email constructor
Email::Email(string sender, string recipient, string emailData)
{
    mailFrom = sender;
    if (!recipient.empty())
    {
        rcptTo.push_back(recipient);
    }
    data = emailData;
    previousState = EmailState::INIT;
}

// Setter and Getter for mailFrom
void Email::setMailFrom(const string &sender)
{
    mailFrom = sender;
}

string Email::getMailFrom() const
{
    return mailFrom;
}

// Setter and Getter for rcptTo
void Email::addRcptTo(const std::string &recipient)
{
    rcptTo.push_back(recipient);
}

vector<string> Email::getRcptTo() const
{
    return rcptTo;
}

// Setter and Getter for data
void Email::setData(const string &emailData)
{
    data = emailData;
}

string Email::getData() const
{
    return data;
}

// Setter and Getter for previousState
void Email::setPreviousState(const EmailState state)
{
    previousState = state;
}

Email::EmailState Email::getPreviousState() const
{
    return previousState;
}

// Displays email information
void Email::displayEmailInfo()
{
    cout << "Mail From: " << mailFrom << endl;
    for (int i = 0; i < rcptTo.size(); i++)
    {
        cout << "Recipient: " << rcptTo[i] << endl;
    }
    cout << "Email Data: " << data << endl;
    cout << "Previous State: " << previousState << endl;
}

std::string FrontendEmailData::createEmailString()
{
    // split FrontendEmailData emailContent into email two parts based on \r\n\r\n
    std::string header = "";
    std::string body_content = "";
    std::string subject_ = "";
    size_t header_end_pos = FrontendEmailData::getEmailData().find("\r\n\r\n");
    if (header_end_pos != std::string::npos)
    {

        header = FrontendEmailData::getEmailData().substr(0, header_end_pos);
        body_content = FrontendEmailData::getEmailData().substr(header_end_pos + 4);
        size_t subject_pos = header.find("Subject:");
        if (subject_pos != std::string::npos)
        {
            size_t subject_end_pos = header.find("\r\n", subject_pos);
            if (subject_end_pos != std::string::npos)
            {
                subject_ = header.substr(subject_pos + 8, subject_end_pos - subject_pos - 8);
                subject_.erase(subject_.find_last_not_of(" \n\r\t") + 1);
                subject_.erase(0, subject_.find_first_not_of(" \n\r\t"));
            }
        }
    }
    std::string email;
    email += "HELO example.com\r\n";
    email += "MAIL FROM:<" + FrontendEmailData::getSender() + ">\r\n";
    email += "RCPT TO:<" + FrontendEmailData::getRecipient() + ">\r\n";
    email += "DATA\r\n";
    email += "Subject: " + subject_ + "\r\n";
    email += body_content + "\r\n";
    email += ".\r\n";
    return email;
}

void Email::process_HELO(const string &domain, int client_fd)
{
    // Prints error if domain is missing
    if (domain.empty())
    {
        string message = "501: Syntax error - missing domain\r\n";
        if (write(client_fd, message.c_str(), message.length()) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd); // Verbose: Server ready message
        }
        return;
    }
    else if (previousState == INIT || previousState == HELO)
    {
        // If previous state is INIT or HELO, set to HELO
        previousState = HELO;
        string message = "250 " + domain + "\r\n";
        if (write(client_fd, message.c_str(), message.length()) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
            // return;
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 250 %s\n", client_fd, domain.c_str()); // Verbose: Server ready message
        }
        return;
    }
    else
    {
        string message = "503 Bad sequence of commands\r\n";
        if (write(client_fd, message.c_str(), message.length()) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 503 bad sequence of commands\n", client_fd); // Verbose: Server ready message
        }
        return;
    }
}

void Email::process_MAILFROM(const string &sender, int client_fd)
{
    if (previousState == HELO)
    {
        size_t colonPos = sender.find(':');
        if (colonPos != string::npos)
        {
            // Splits the string on ':'
            string command = sender.substr(0, colonPos);
            string addressPart = sender.substr(colonPos + 1);

            // Trims leading and trailing whitespace from command
            size_t cmdStart = command.find_first_not_of(" \t");
            size_t cmdEnd = command.find_last_not_of(" \t");
            if (cmdStart == string::npos || cmdEnd == string::npos)
            {
                string message = "501 Syntax error\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }
            command = command.substr(cmdStart, cmdEnd - cmdStart + 1);

            // Converts command to uppercase for case-insensitive comparison
            transform(command.begin(), command.end(), command.begin(), ::toupper);

            // Validates that the command is 'FROM'
            if (command != "FROM")
            {
                string message = "501 Syntax error\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                    // exit(1);
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }

            // Trims leading and trailing whitespace from addressPart
            size_t addrStart = addressPart.find_first_not_of(" \t");
            size_t addrEnd = addressPart.find_last_not_of(" \t");

            if (addrStart == string::npos || addrEnd == string::npos)
            {
                string message = "501 Syntax error\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                    // exit(1);
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }
            string address = addressPart.substr(addrStart, addrEnd - addrStart + 1);

            // Checks if address is enclosed in '<' and '>'
            if (address.front() != '<' || address.back() != '>')
            {
                string message = "501 Syntax error\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                    // exit(1);
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }

            // Extracts the email address between '<' and '>'
            string email = address.substr(1, address.size() - 2);

            // Validates the email format (basic validation)
            if (!isValidEmail(email))
            {
                string message = "501 Syntax error: invalid email\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error-invalid email\n", client_fd);
                }
                return;
            }

            // Sets mailFrom and updates the state
            globalEmail.setMailFrom(email);
            mailFrom = email;
            previousState = MAIL;

            string message = "250 OK\r\n";
            if (write(client_fd, message.c_str(), message.length()) < 0)
            {
                fprintf(stderr, "Could not communicate with client\r\n");
            }
            if (verbose)
            {
                fprintf(stderr, "[%d] S: 250 OK\n", client_fd);
            }
        }
        else
        {
            //':' not found in sender string
            string message = "501 Syntax error: missing :\r\n";
            if (write(client_fd, message.c_str(), message.length()) < 0)
            {
                fprintf(stderr, "Could not communicate with client\r\n");
            }
            if (verbose)
            {
                fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
            }
        }
    }
    else
    {
        string message = "503 Bad sequence of commands\r\n";
        if (write(client_fd, message.c_str(), message.length()) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 503 Bad sequence of commands\n", client_fd);
        }
    }
}

void Email::process_RCPTTO(const string &recipient, int client_fd, string mail_dir)
{
    if (previousState == MAIL || previousState == RCPT)
    {
        // Checks if recipient contains ':'
        size_t colonPos = recipient.find(':');
        if (colonPos != string::npos)
        {
            string command = recipient.substr(0, colonPos);
            string addressPart = recipient.substr(colonPos + 1);

            // Trims leading and trailing whitespace from command
            size_t cmdStart = command.find_first_not_of(" \t");
            size_t cmdEnd = command.find_last_not_of(" \t");
            if (cmdStart == string::npos || cmdEnd == string::npos)
            {
                string message = "501 Syntax error - wrong command format\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }
            command = command.substr(cmdStart, cmdEnd - cmdStart + 1);

            // Converts command to uppercase for case-insensitive comparison
            transform(command.begin(), command.end(), command.begin(), ::toupper);

            // Validates that the command is 'TO'
            if (command != "TO")
            {
                string message = "501 Syntax error - missing TO\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                    // exit(1);
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }

            // Trims leading and trailing whitespace from addressPart
            size_t addrStart = addressPart.find_first_not_of(" \t");
            size_t addrEnd = addressPart.find_last_not_of(" \t");
            if (addrStart == string::npos || addrEnd == string::npos)
            {
                string message = "501 Syntax error - incorrect address\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }
            string address = addressPart.substr(addrStart, addrEnd - addrStart + 1);

            // Checks if address is enclosed in '<' and '>'
            if (address.front() != '<' || address.back() != '>')
            {
                string message = "501 Syntax error - malformed email\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }

            // Extracts the email address between '<' and '>'
            string email = address.substr(1, address.size() - 2);

            if (!isValidEmail(email))
            {
                string message = "501 Syntax error - invalid email\r\n";
                if (write(client_fd, message.c_str(), message.length()) < 0)
                {
                    fprintf(stderr, "Could not communicate with client\r\n");
                    // exit(1);
                }
                if (verbose)
                {
                    fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
                }
                return;
            }
            size_t atPos = email.find('@');
            string username = email.substr(0, atPos);

            // Appends recipient to the rcptTo vector and update the state
            rcptTo.push_back(email);
            previousState = RCPT;

            // Respond with success
            string message = "250 OK\r\n";
            if (write(client_fd, message.c_str(), message.length()) < 0)
            {
                fprintf(stderr, "Could not communicate with client\r\n");
            }
            if (verbose)
            {
                fprintf(stderr, "[%d] S: 250 OK\n", client_fd);
            }
            return;
        }
        else
        {
            //':' not found in recipient string
            string message = "501 Syntax error - missing :\r\n";
            if (write(client_fd, message.c_str(), message.length()) < 0)
            {
                fprintf(stderr, "Could not communicate with client\r\n");
            }
            if (verbose)
            {
                fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
            }
            return;
        }
    }
    else
    {
        // Invalid sequence of commands
        string message = "503 Bad sequence of commands\r\n";
        if (write(client_fd, message.c_str(), message.length()) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 503 Bad sequence of commands\n", client_fd);
        }
        return;
    }
}

bool Email::isValidEmail(const string &email) const
{
    // Basic validation: checks for presence of '@'
    size_t atPos = email.find('@');
    return (atPos != string::npos);
}

void Email::process_DATA(int &client_fd, string argument, string mail_dir)
{
    cout << "in process_data" << endl;

    // Checks if argument is not empty
    GRPCCalls grpcCalls;
    // vector of addresses
    vector<string> kvsAddresses;
    if (!argument.empty())
    {
        const char *syntax_error_msg = "501 Syntax error: Remove argument\r\n";
        if (write(client_fd, syntax_error_msg, strlen(syntax_error_msg)) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
        }
        return;
    }

    if (previousState != RCPT)
    {
        // Invalid sequence of commands
        string message = "503 Bad sequence of commands\r\n";
        if (write(client_fd, message.c_str(), message.length()) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 503 Bad sequence of commands\n", client_fd);
        }
    }

    // Prompts the user to start entering email data
    string message = "354 Start mail input; end with <CRLF>.<CRLF>\r\n";
    if (write(client_fd, message.c_str(), message.length()) < 0)
    {
        fprintf(stderr, "Could not communicate with client\r\n");
    }
    if (verbose)
    {
        fprintf(stderr, "[%d] S: 354 Start mail input; end with <CRLF>.<CRLF>\n", client_fd);
    }

    string emailData;
    char recv_buffer[1024];
    ssize_t bytes_received;
    while (true)
    {
        // Receives data from the client
        bytes_received = recv(client_fd, recv_buffer, sizeof(recv_buffer), 0);
        cout << "receiving client data" << endl;
        if (bytes_received == -1)
        {
            fprintf(stderr, "read failed\n");
            return;
        }
        else if (bytes_received == 0)
        {
            fprintf(stderr, "Client disconnected\n");
            return;
        }
        emailData.append(recv_buffer, bytes_received);

        // Checks if the termination sequence\r\n.\r\n is present
        size_t term_pos = emailData.find("\r\n.\r\n");
        if (term_pos != string::npos)
        {
            // Removes the termination sequence from emailData
            emailData.erase(term_pos);
            break;
        }
    }

    cout << "emailData " << emailData << endl;
    if (!emailData.empty() && emailData.back() != '\n')
    {
        emailData.append("\r\n");
    }
    data = emailData;
    previousState = DATA;

    string sender = globalEmail.getMailFrom();

    time_t now = time(0);
    struct tm *now_tm = localtime(&now);
    stringstream ss;
    ss << put_time(now_tm, "%Y-%m-%d-%H:%M:%S");
    string formatted_time = ss.str();

    string id = generate_id();
    string emailId = formatted_time + "#" + id;
    string col_base_name = id + "#data";
    int num_chunks = 0;

    for (const string &recipient : rcptTo)
    {
        size_t atPos = recipient.find('@');
        if (atPos == string::npos)
        {
            cerr << "Invalid email format: " << recipient << endl;
        }
        string username = recipient.substr(0, atPos);
        string kvsAddress = grpcCalls.getKVSAddress(username);

        // Create the row key: username#email
        string rowKey = username + "#email";
        cout << "rowKey " << rowKey << endl;

        Client client(kvsAddress);
        // Get list from column rcvdEmails in the KV Store
        string rcvdEmailsValue;
        bool exists = client.Get(rowKey, "rcvdEmails", rcvdEmailsValue);
        if (!exists)
        {
            rcvdEmailsValue = "";
        }
        // cout << "rcvdEmails " << rcvdEmailsValue << endl;

        // Parse the retrieved list into a vector
        vector<string> emailList = parseEmailList(rcvdEmailsValue);

        // cout << "email_id: " << id << endl;

        // Append email_id to the list
        emailList.push_back(id);

        for (string email : emailList)
        {
            cout << email << " ";
        }
        cout << endl;

        // Serialize the updated email list
        string updatedRcvdEmailsValue = serializeEmailList(emailList);
        // cout << "updatedRcvdEmailsValue " << updatedRcvdEmailsValue << endl;

        // Update the rcvdEmails column in the KV Store
        if (!client.Put(rowKey, "rcvdEmails", updatedRcvdEmailsValue))
        {
            cerr << "Failed to update rcvdEmails for " << rowKey << endl;
            // continue;
        }

        if (sender == recipient)
            continue;
        
        stringstream emailObject;
        emailObject << "mailFrom:" + mailFrom + "\n"
                                                "time:" +
                           formatted_time + "\n" + "data:" + data + "\n";
        
        // Use the reusable chunking function
        if (!chunk_and_store(rowKey, col_base_name, emailObject.str(), num_chunks, kvsAddress))
        {
            string errorMessage = "550 Failed to store email data\r\n";
            if (write(client_fd, errorMessage.c_str(), errorMessage.length()) < 0)
            {
                fprintf(stderr, "Could not communicate with client\r\n");
            }
            return;
        }

        // Store metadata
        stringstream metadata;
        metadata << formatted_time << "#" << num_chunks << "#" << sender << "#" << recipient << "#";

        if (!client.Put(rowKey, id + "#metadata", metadata.str()))
        {
            string errorMessage = "550 Failed to store email metadata\r\n";
            if (write(client_fd, errorMessage.c_str(), errorMessage.length()) < 0)
            {
                fprintf(stderr, "Could not communicate with client\r\n");
            }
            fprintf(stderr, "Failed to store email metadata\n");
            return;
        }

        cout << "Processed email for recipient: " << recipient << endl;

        mailFrom.clear();
        rcptTo.clear();
        data.clear();
        previousState = HELO;
    }
    string success_message = "250 OK\r\n";
    if (write(client_fd, success_message.c_str(), success_message.length()) < 0)
    {
        fprintf(stderr, "Could not communicate with client\r\n");
    }
    if (verbose)
    {
        fprintf(stderr, "[%d] S: 250 OK\n", client_fd);
    }
}

// Generates unique ID for an email
string Email::generate_id()
{
    // random number generator
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 35); // Values from 0-9 and a-z

    const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    string email_id;
    for (int i = 0; i < 32; ++i)
    { // Generate 32-character session_id
        email_id += chars[dis(gen)];
    }
    return email_id;
}

/*
row_name: username#email
col_base_name: email_id#data
*/
bool Email::chunk_and_store(const string &row_name, const string &col_base_name, const string &data, int32_t &num_chunks, 
    string &server_address)
{
    // std::cout << "Address: " << server_address << std::endl;
    Client client(server_address);
    num_chunks = 0; // Reset chunk count

    string remaining_data = data;
    while (!remaining_data.empty())
    {
        // Extract a chunk
        string chunk = remaining_data.substr(0, CHUNK_SIZE);
        remaining_data.erase(0, CHUNK_SIZE);
        num_chunks++;

        // Store the chunk in KVS
        string col_name = col_base_name + "#" + to_string(num_chunks);
        if (!client.Put(row_name, col_name, chunkDecode(chunk)))
        {
            cerr << "Failed to store chunk " << num_chunks << endl;
            return false;
        }
    }

    return true;
}

std::string Email::chunkDecode(const std::string &encoded)
{
    std::ostringstream decoded;
    for (size_t i = 0; i < encoded.length(); ++i)
    {
        if (encoded[i] == '%' && i + 2 < encoded.length())
        {
            std::string hexValue = encoded.substr(i + 1, 2);
            if (isxdigit(hexValue[0]) && isxdigit(hexValue[1]))
            {
                int charCode;
                std::istringstream(hexValue) >> std::hex >> charCode;
                decoded << static_cast<char>(charCode);
                i += 2; // Skip next two characters
            }
            else
            {
                decoded << '%'; // Leave the '%' as-is if invalid
            }
        }
        else if (encoded[i] == '+')
        {
            decoded << ' ';
        }
        else
        {
            decoded << encoded[i];
        }
    }
    return decoded.str();
}

// Parse a comma-separated list of emails into a vector
vector<string> Email::parseEmailList(string &emailListStr)
{
    vector<string> emailList;
    stringstream ss(emailListStr);
    string item;
    while (getline(ss, item, ','))
    {
        if (!item.empty())
        {
            emailList.push_back(item);
        }
    }
    return emailList;
}

// Serializes a vector of emails into a comma-separated string
string Email::serializeEmailList(vector<string> &emailList)
{
    stringstream ss;
    for (size_t i = 0; i < emailList.size(); i++)
    {
        ss << emailList[i];
        if (i != emailList.size() - 1)
        {
            ss << ",";
        }
    }
    return ss.str();
}

void Email::process_RSET(int &client_fd, string argument)
{
    // Checks if an argument is provided
    if (!argument.empty())
    {
        const char *syntax_error_msg = "501 Syntax error: Remove argument\r\n";
        if (write(client_fd, syntax_error_msg, strlen(syntax_error_msg)) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
            // exit(1);
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
        }
        return;
    }

    // Checks if the previous state is INIT
    if (previousState == INIT)
    {
        const char *bad_sequence_msg = "503 Bad sequence of commands\r\n";
        if (write(client_fd, bad_sequence_msg, strlen(bad_sequence_msg)) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
            // exit(1);
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 503 Bad sequence of commands\n", client_fd);
        }
        return;
    }

    // Clears all stored sender, recipients, and mail data
    mailFrom.clear();
    rcptTo.clear();
    data.clear();
    previousState = HELO;

    // Sends 250 OK to the client
    const char *success_msg = "250 OK\r\n";
    if (write(client_fd, success_msg, strlen(success_msg)) < 0)
    {
        fprintf(stderr, "Could not communicate with client\r\n");
    }
    if (verbose)
    {
        fprintf(stderr, "[%d] S: 250 OK\n", client_fd);
    }
}

void Email::process_QUIT(int &client_fd, string argument)
{
    // Checks if an argument is provided
    if (!argument.empty())
    {
        const char *syntax_error_msg = "501 Syntax error: Remove argument\r\n";
        if (write(client_fd, syntax_error_msg, strlen(syntax_error_msg)) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
            // exit(1);
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
        }
        return;
    }

    // Sends 250 OK to the client
    const char *success_msg = "221 localhost closing transmission\r\n";
    if (write(client_fd, success_msg, strlen(success_msg)) < 0)
    {
        fprintf(stderr, "Could not communicate with client\r\n");
    }
    if (verbose)
    {
        fprintf(stderr, "[%d] S: 221 localhost closing transmission\n", client_fd);
    }

    // Clears all stored sender, recipients, and mail data
    mailFrom.clear();
    rcptTo.clear();
    data.clear();
    previousState = INIT;

    // Closes the client connection
    close(client_fd);
    pthread_exit(nullptr);
}

void Email::process_NOOP(int &client_fd, string argument)
{
    // Checks if an argument is provided
    if (!argument.empty())
    {
        const char *syntax_error_msg = "501 Syntax error: Remove argument\r\n";
        if (write(client_fd, syntax_error_msg, strlen(syntax_error_msg)) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 501 Syntax error\n", client_fd);
        }
        return;
    }

    // Checks if the previous state is not INIT
    if (previousState != INIT)
    {
        const char *bad_sequence_msg = "503 Bad sequence of commands\r\n";
        if (write(client_fd, bad_sequence_msg, strlen(bad_sequence_msg)) < 0)
        {
            fprintf(stderr, "Could not communicate with client\r\n");
        }
        if (verbose)
        {
            fprintf(stderr, "[%d] S: 503 Bad sequence of commands\n", client_fd);
        }
        return;
    }

    // Sends 250 OK to the client
    const char *success_msg = "250 OK\r\n";
    if (write(client_fd, success_msg, strlen(success_msg)) < 0)
    {
        fprintf(stderr, "Could not communicate with client\r\n");
    }
    if (verbose)
    {
        fprintf(stderr, "[%d] S: 250 OK\n", client_fd);
    }
}
// setter for sender
void FrontendEmailData::setSender(const string &sender_)
{
    sender = sender_;
}
// setter for recipient
void FrontendEmailData::setRecipient(const string &recipient_)
{
    recipient = recipient_;
}
// setter for emailData
void FrontendEmailData::setEmailData(const string &emailData_)
{
    emailData = emailData_;
}
// setter for num_chunks
void FrontendEmailData::setNumChunks(int32_t num_chunks_)
{
    num_chunks = num_chunks_;
}
// setter for server_address
void FrontendEmailData::setServerAddress(const string &server_address)
{
    server_address_ = server_address;
}
// getter for sender
string FrontendEmailData::getSender()
{
    return sender;
}
// getter for recipient
string FrontendEmailData::getRecipient()
{
    return recipient;
}
// getter for emailData
string FrontendEmailData::getEmailData()
{
    return emailData;
}
// getter for num_chunks
int32_t FrontendEmailData::getNumChunks()
{
    return num_chunks;
}
// getter for server_address
string FrontendEmailData::getServerAddress()
{
    return server_address;
}
// function to store email data in the backend
void FrontendEmailData::storeEmailData()
{

    // usename is everything before the @ in the sender email FrontendEmailData::getSender();
    string username = "";
    string domain = "";
    string sender = FrontendEmailData::getSender();

    // check if there are multiple recipients in the recipient field

    string recipient = FrontendEmailData::getRecipient();
    vector<string> recipients;
    stringstream ss(recipient);
    string item;
    while (getline(ss, item, ','))
    {
        if (!item.empty())
        {
            recipients.push_back(item);
        }
    }
    std::string emailData = FrontendEmailData::getEmailData();
    Email email(sender, recipient, emailData);
    string id = email.generate_id();
    // std::cout << "id: " << id << std::endl;
    size_t atPos = sender.find('@');
    if (atPos != std::string::npos)
    {
        username = sender.substr(0, atPos);
        domain = sender.substr(atPos + 1);
    }
    // std::cout << "username: " << username << std::endl;
    // std::cout << "sender: " << sender << std::endl;
    // std::cout << "domain: " << domain << std::endl;
    GRPCCalls grpc_call;
    std::string kvsAddress = grpc_call.getKVSAddress(username);

    std::cout << "kvsAddress: " << kvsAddress << std::endl;
    // =========================
    std::string temp = kvsAddress;
    std::cout << "Temp: " << temp << std::endl;

    Client client(temp);
    time_t now = time(0);
    struct tm *now_tm = localtime(&now);
    stringstream st;
    st << put_time(now_tm, "%a %b %d %H:%M:%S %Y");
    string formatted_time = st.str();

    if (sender.find("@penncloud") != string::npos)
    {
        // Create the row key: username#email
        string senderUsername = username;
        string rowKey = senderUsername + "#email";
        
        // Get list from unique IDs from column sentEmails in the KV Store
        string sentEmailsValue;
        bool exists = client.Get(rowKey, "sentEmails", sentEmailsValue);
        if (!exists)
        {
            sentEmailsValue = "";
        }
        // cout << "sentEmails: " << sentEmailsValue << endl;
        // Parse the retrieved list into a vector
        vector<string> emailList = email.parseEmailList(sentEmailsValue);
        
        // Append email_id to the list
        emailList.push_back(id);
        for (string email : emailList)
        {
            cout << email << " ";
        }
        cout << endl;
        // Serialize the updated email list
        string updatedSentEmailsValue = email.serializeEmailList(emailList);
        // cout << "updatedSentEmailsValue " << updatedSentEmailsValue << endl;

        // Update the sentEmails column in the KV Store
        if (!client.Put(rowKey, "sentEmails", updatedSentEmailsValue))
        {
            cerr << "Failed to update sentEmails for " << rowKey << endl;
            // continue;
        }
        int32_t num_chunks = FrontendEmailData::getNumChunks();

        if (!email.chunk_and_store(rowKey, id + "#data", emailData, num_chunks, temp))
        {
            string errorMessage = "550 Failed to store email data\r\n";
            cout << errorMessage << endl;
            return;
        }
        // Store metadata
        stringstream metadata;
        metadata << formatted_time << "#" << num_chunks << "#" << sender << "#" << recipient << "#";
        if (!client.Put(rowKey, id + "#metadata", metadata.str()))
        {
            string errorMessage = "550 Failed to store email metadata\r\n";
            cout << errorMessage << endl;
            fprintf(stderr, "Failed to store email metadata\n");
            return;
        }
    }
    // iterate through all recipients for storing the mail data for each of them
    for (const string &recipient : recipients)
    {
        size_t atPos = recipient.find('@');
        if (atPos == string::npos)
        {
            cerr << "Invalid email format: " << recipient << endl;
            break;
        }
        string username = recipient.substr(0, atPos);
        // get kvs address for recipient
        std::string kvsAddressRecipient = grpc_call.getKVSAddress(username);
        Client clientRecvd(kvsAddressRecipient);
        // std::cout << "kvsAddressRecipient: " << kvsAddressRecipient << std::endl;
        // if recipient is not a penncloud user
        if (recipient.find("@penncloud") == string::npos)
        {
            try
            {
                // Initialize the gRPC stub
                auto channel = grpc::CreateChannel("localhost:50060", grpc::InsecureChannelCredentials());
                unique_ptr<SMTPService::Stub> stub = SMTPService::NewStub(channel);

                // Prepare the request
                SendEmailRequest request;
                request.set_sender(globalEmail.getMailFrom());
                request.set_recipient(recipient);
                request.set_email_object(emailData);

                // Prepare the response and context
                SendEmailResponse response;
                ClientContext context;

                // Make the RPC call
                cout << "SENT EMAIL REQUEST " << endl;
                Status status = stub->SendEmail(&context, request, &response);

                // Handle the response
                cout << "HANDLING EMAIL RESPONSE " << endl;
                if (status.ok())
                {
                    if (response.success())
                    {
                        cout << "Email sent successfully: " << response.message() << endl;
                    }
                    else
                    {
                        throw runtime_error("Failed to send email: " + response.message());
                    }
                }
                else
                {
                    throw runtime_error("gRPC call failed: " + status.error_message());
                }
            }
            catch (const exception &e)
            {
                // Handle errors and respond back to the client
                string errorMessage = "550 Failed to send email\r\n";
                cout << errorMessage << endl;
            }
            continue;
        }

        // Create the row key: username#email
        string rowKey = username + "#email";
        // std::cout << "rowKey " << rowKey << endl;

        // Get list from column rcvdEmails in the KV Store
        string rcvdEmailsValue;
        bool exists = clientRecvd.Get(rowKey, "rcvdEmails", rcvdEmailsValue);
        if (!exists)
        {
            rcvdEmailsValue = "";
        }
        // std::cout << "rcvdEmails " << rcvdEmailsValue << endl;

        // Parse the retrieved list into a vector
        vector<string> emailList = email.parseEmailList(rcvdEmailsValue);

        // std::cout << "email_id: " << id << std::endl;

        // Append email_id to the list
        emailList.push_back(id);

        for (string email : emailList)
        {
            cout << email << " ";
        }
        cout << endl;

        // Serialize the updated email list
        string updatedRcvdEmailsValue = email.serializeEmailList(emailList);
        // cout << "updatedRcvdEmailsValue " << updatedRcvdEmailsValue << endl;

        // Update the rcvdEmails column in the KV Store
        if (!clientRecvd.Put(rowKey, "rcvdEmails", updatedRcvdEmailsValue))
        {
            cerr << "Failed to update rcvdEmails for " << rowKey << endl;
            // continue;
        }
        // Use the reusable chunking function
        if (!email.chunk_and_store(rowKey, id + "#data", emailData, num_chunks, kvsAddressRecipient))
        {
            string errorMessage = "550 Failed to store email data\r\n";
            cout << errorMessage << endl;
            return;
        }

        // Store metadata
        stringstream metadata;
        metadata << formatted_time << "#" << num_chunks << "#" << sender << "#" << recipient << "#";

        if (!clientRecvd.Put(rowKey, id + "#metadata", metadata.str()))
        {
            string errorMessage = "550 Failed to store email metadata\r\n";
            cout << errorMessage << endl;
            fprintf(stderr, "Failed to store email metadata\n");
            return;
        }
    }
}