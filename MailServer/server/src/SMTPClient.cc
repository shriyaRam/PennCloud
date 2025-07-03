#include <grpcpp/grpcpp.h>
#include "proto/coordinator.grpc.pb.h"
#include "proto/coordinator.pb.h"
#include "proto/smtpClient.grpc.pb.h"
#include "proto/smtpClient.pb.h"
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <resolv.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using smtpClient::SendEmailRequest;
using smtpClient::SendEmailResponse;
using smtpClient::SMTPService;

class SMTPServiceImpl final : public SMTPService::Service
{
private:
    // Extracts domain of external user
    string extractDomain(const string &recipient)
    {
        auto atPos = recipient.find('@');
        if (atPos == string::npos)
        {
            throw invalid_argument("Invalid email address format.");
        }
        cout << "in extractDomain " << recipient.substr(atPos + 1) << endl;
        return recipient.substr(atPos + 1);
    }

    // Obtains the MX record of the external server from the DNS server
    string querySMTPServer(const string &domain)
    {
        // cout << "in querySMTPServer domain: " << domain << endl;
        unsigned char response[NS_PACKETSZ];
        int len = res_query(domain.c_str(), ns_c_in, ns_t_mx, response, sizeof(response));
        if (len < 0)
        {
            throw runtime_error("DNS query failed.");
        }
        // cout << "len " << len << endl;
        ns_msg handle;
        if (ns_initparse(response, len, &handle) < 0)
        {
            throw runtime_error("DNS response parsing failed.");
        }

        ns_rr record;
        for (int i = 0; i < ns_msg_count(handle, ns_s_an); ++i)
        {
            if (ns_parserr(&handle, ns_s_an, i, &record) < 0)
            {
                throw runtime_error("Failed to parse DNS record.");
            }

            if (ns_rr_type(record) == ns_t_mx)
            {
                char mxName[NS_MAXDNAME];
                if (dn_expand(ns_msg_base(handle), ns_msg_end(handle), ns_rr_rdata(record) + 2, mxName, sizeof(mxName)) < 0)
                {
                    throw runtime_error("Failed to expand MX record.");
                }
                // cout << "string(mxName) " << string(mxName) << endl;
                return string(mxName);
            }
        }

        // cout << "in querySMTPServer " << endl;
        throw runtime_error("No MX records found.");
    }

    // Extracts the domain name from the MX record
    string resolveIPAddress(const string &hostname)
    {
        // cout << "in resolveIPAddress hostname: " << hostname << endl;
        struct addrinfo hints
        {
        }, *res;
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(hostname.c_str(), nullptr, &hints, &res) != 0)
        {
            throw runtime_error("Failed to resolve IP address.");
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(((struct sockaddr_in *)res->ai_addr)->sin_addr), ipStr, INET_ADDRSTRLEN);

        freeaddrinfo(res);
        return string(ipStr);
    }

    // Sends email to the external SMTP server
    void sendToSMTPServer(const string &ip, const string &sender, const string &recipient, const string &emailObject)
    {
        // cout << " in send To SMTP Server " << endl;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            throw runtime_error("Socket creation failed.");
        }
        // cout << " created socket " << endl;

        struct sockaddr_in serverAddr
        {
        };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(25); // SMTP port
        inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
        // cout << " initialized socket " << endl;

        if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            close(sockfd);
            throw runtime_error("Connection to SMTP server failed.");
        }
        // cout << " connected socket " << endl;

        auto readServerResponse = [&](string &response, int timeoutSecs) -> bool
        {
            char buffer[1024] = {0};
            fd_set readfds;
            struct timeval timeout
            {
            };
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            timeout.tv_sec = timeoutSecs;
            timeout.tv_usec = 0;

            int ret = select(sockfd + 1, &readfds, nullptr, nullptr, &timeout);
            if (ret > 0 && FD_ISSET(sockfd, &readfds))
            {
                int bytesRead = read(sockfd, buffer, sizeof(buffer) - 1);
                if (bytesRead > 0)
                {
                    response = string(buffer, bytesRead);
                    return true;
                }
            }
            return false;
        };

        // cout << " after readServerResponse " << endl;
        string response;

        // Wait for initial response from server
        cout << "// Wait for initial response from server" << endl;
        if (!readServerResponse(response, 5) || response.find("220") == string::npos)
        {
            close(sockfd);
            throw runtime_error("Failed to send email: No response from SMTP server.");
        }
        cout << "response " << response << endl;

        // Send HELO command
        cout << "// Send HELO command " << endl;
        string smtpCommands = "HELO localhost\r\n";
        if (send(sockfd, smtpCommands.c_str(), smtpCommands.size(), 0) < 0)
        {
            close(sockfd);
            throw runtime_error("Failed to send HELO command.");
        }
        if (!readServerResponse(response, 5) || response.find("250") == string::npos)
        {
            close(sockfd);
            throw runtime_error("Failed to send email: HELO command not acknowledged.");
        }
        cout << "response " << response << endl;

        // Send MAIL FROM command
        cout << "// Send MAIL FROM command " << endl;
        smtpCommands = "MAIL FROM:<" + sender + ">\r\n";
        if (send(sockfd, smtpCommands.c_str(), smtpCommands.size(), 0) < 0)
        {
            close(sockfd);
            throw runtime_error("Failed to send MAIL FROM command.");
        }
        if (!readServerResponse(response, 5) || response.find("250") == string::npos)
        {
            close(sockfd);
            throw runtime_error("Failed to send email: MAIL FROM command not acknowledged.");
        }
        cout << "response " << response << endl;

        // Send RCPT TO command
        cout << "// Send RCPT TO command " << endl;
        smtpCommands = "RCPT TO:<" + recipient + ">\r\n";
        if (send(sockfd, smtpCommands.c_str(), smtpCommands.size(), 0) < 0)
        {
            close(sockfd);
            throw runtime_error("Failed to send RCPT TO command.");
        }
        if (!readServerResponse(response, 5) || response.find("250") == string::npos)
        {
            close(sockfd);
            throw runtime_error("Failed to send email: RCPT TO command not acknowledged.");
        }
        cout << "response " << response << endl;

        // Send DATA command
        cout << "// Send DATA command " << endl;
        smtpCommands = "DATA\r\n";
        if (send(sockfd, smtpCommands.c_str(), smtpCommands.size(), 0) < 0)
        {
            close(sockfd);
            throw runtime_error("Failed to send DATA command.");
        }
        if (!readServerResponse(response, 5) || response.find("354") == string::npos)
        {
            close(sockfd);
            throw runtime_error("Failed to send email: DATA command not acknowledged.");
        }
        cout << "response " << response << endl;

        // Send email body and end with a single period on a line
        cout << "// Send email body and end with a single period on a line " << endl;
        smtpCommands = emailObject + "\r\n.\r\n";
        if (send(sockfd, smtpCommands.c_str(), smtpCommands.size(), 0) < 0)
        {
            close(sockfd);
            throw runtime_error("Failed to send email body.");
        }
        cout << "// Sent email body " << endl;
        if (!readServerResponse(response, 20) || response.find("250") == string::npos)
        {
            cout << "response " << response << endl;
            close(sockfd);
            throw runtime_error("Failed to send email: Email body not acknowledged.");
        }
        cout << "response " << response << endl;

        // Send QUIT command
        cout << "// Send QUIT command " << endl;
        smtpCommands = "QUIT\r\n";
        if (send(sockfd, smtpCommands.c_str(), smtpCommands.size(), 0) < 0)
        {
            close(sockfd);
            throw runtime_error("Failed to send QUIT command.");
        }
        // cout << "response " << response << endl;
        if (!readServerResponse(response, 5) || response.find("250") == string::npos)
        {
            close(sockfd);
            throw runtime_error("Quit not acknowledged.");
        }
        cout << "response " << response << endl;

        close(sockfd);
        cout << "socket closed " << endl;
    }

public:
    Status SendEmail(ServerContext *context, const SendEmailRequest *request, SendEmailResponse *response) override
    {
        // cout << "IN SendEmail " << endl;
        try
        {
            // Extract sender, recipient, and email object from the request
            string sender = request->sender();
            string recipient = request->recipient();
            string emailObject = request->email_object();
            cout << "in send email sender: " << sender << " recipient: " << recipient << " emailObject: " << emailObject << endl;

            // Implement email sending logic
            string domain = extractDomain(recipient);
            string smtpServer = querySMTPServer(domain);
            string ip = resolveIPAddress(smtpServer);
            cout << "in send email domain: " << domain << " smtpServer: " << smtpServer << " ip: " << ip << endl;
            sendToSMTPServer(ip, sender, recipient, emailObject);
            cout << "back in SendEmail " << endl;
            // Set response fields
            response->set_success(true);
            cout << "set success message " << endl;
            response->set_message("Email sent successfully.");
            cout << "set email meesage " << endl;
        }
        catch (const exception &e)
        {
            cout << "exception occurred in sendEmail " << endl;
            // Set response fields on error
            response->set_success(false);
            response->set_message(e.what());
        }

        return Status::OK;
    }
};

// Function to start the server
void RunServer()
{
    string server_address("0.0.0.0:50060"); // Listen on port 50060
    SMTPServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "gRPC Server is running on " << server_address << endl;

    server->Wait(); // Block until the server shuts down
}

int main()
{
    RunServer();
    return 0;
}