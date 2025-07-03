#include <iostream>
#include <string>
#include <ctime>
#include "EmailClient.h"

using namespace std;

const size_t CHUNK_SIZE = 1024 * 1024 * 3.9;

std::string generate_headers(const std::string &sender, const std::string &receiver, std::string subject)
{
    // Get the current time
    std::time_t now = std::time(nullptr);
    char date_buffer[100];
    std::strftime(date_buffer, sizeof(date_buffer), "%a, %d %b %Y %H:%M:%S %z", std::localtime(&now));
    std::string date(date_buffer);

    std::string username = sender.substr(0, sender.find("@"));

    // Generate the header
    std::string header;
    header += "Date: " + date + "\r\n";
    header += "MIME-Version: 1.0\r\n";
    header += "User-Agent: Mozilla Thunderbird\r\n";
    header += "Content-Language: en-US\r\n";
    header += "To: " + receiver + "\r\n";
    header += "From: " + username + " <" + sender + ">\r\n";
    header += "Subject: " + subject + "\r\n";
    header += "Content-Type: text/plain; charset=UTF-8; format=flowed\r\n";
    header += "Content-Transfer-Encoding: 7bit\r\n\r\n";

    return header;
}

bool send_email_chunks(std::string &content, const std::string &server_address, std::string from_email, std::string to_email)
{
    // Send the email in chunks
    std::cout << "SERVER Address in email chunks: " << server_address << std::endl;
    EmailClient client(server_address);

    // Split the content into chunks
    int num_chunks = content.size() / CHUNK_SIZE;
    std::cout << "Num chunks: " << num_chunks << std::endl;
    if (content.size() % CHUNK_SIZE != 0)
    {
        num_chunks++;
    }
    while (content.size() > CHUNK_SIZE)
    {
        std::string chunk = content.substr(0, CHUNK_SIZE);
        content = content.substr(CHUNK_SIZE);
        std::cout << "Calling compose email" << std::endl;
        if (!client.ComposeEmail(from_email, to_email, chunk, num_chunks))
        {
            return false;
        }
    }
    // Send the last chunk
    if (!client.ComposeEmail(from_email, to_email, content, num_chunks))
    {
        return false;
    }
    std::cout << "Email sent successfully in send_email_chunks" << std::endl;
    return true;
}

bool send_email(std::string &from_email, std::string &to_email, std::string &subject, std::string &content, std::string &server_address)
{
    std::cout << "In send email " << std::endl;
    std::string header = generate_headers(from_email, to_email, subject);
    content += "\r\n.\r\n";
    content = header + content;
    if (!send_email_chunks(content, server_address, from_email, to_email))
    {
        std::cerr << "Failed to send email" << std::endl;
        return false;
    }
    std::cout << "Email sent successfully in send_email" << std::endl;
    return true;
}

bool forward_email_chunks(std::string &content, const std::string &server_address, std::string from_email, std::string to_email)
{
    // Send the email in chunks
    EmailClient client(server_address);

    // Split the content into chunks
    int num_chunks = content.size() / CHUNK_SIZE;
    if (content.size() % CHUNK_SIZE != 0)
    {
        num_chunks++;
    }
    while (content.size() > CHUNK_SIZE)
    {
        std::string chunk = content.substr(0, CHUNK_SIZE);
        content = content.substr(CHUNK_SIZE);
        if (!client.ComposeEmail(from_email, to_email, chunk, num_chunks))
        {
            return false;
        }
    }
    // Send the last chunk
    if (!client.ComposeEmail(from_email, to_email, content, num_chunks))
    {
        return false;
    }
    return true;
}

bool forward_email(std::string &from_email, std::string &to_email, std::string &subject, std::string &content, std::string &server_address)
{

    std::string header = generate_headers(from_email, to_email, subject);
    content += "\r\n.\r\n";
    content = header + content;
    if (!forward_email_chunks(content, server_address, from_email, to_email))
    {
        std::cerr << "Failed to send email" << std::endl;
        return false;
    }
    return true;
}

// timestamp#number_of_chunks#sender#recipient#
unordered_map<string, string> parse_email_metadata(std::string metadata)
{
    std::cout << "Metadata: " << metadata << std::endl;
    unordered_map<string, string> metadata_map;
    size_t pos = metadata.find("#");
    metadata_map["timestamp"] = metadata.substr(0, pos);
    metadata = metadata.substr(pos + 1);
    pos = metadata.find("#");
    metadata_map["number_of_chunks"] = metadata.substr(0, pos);
    metadata = metadata.substr(pos + 1);
    pos = metadata.find("#");
    metadata_map["sender"] = metadata.substr(0, pos);
    metadata = metadata.substr(pos + 1);
    metadata.erase(metadata.size() - 1); // remove the last #
    metadata_map["recipient"] = metadata;
    return metadata_map;
}

std::string decodeTabComponent(const std::string &encoded)
{
    std::string decoded;
    char temp[3] = {0}; // Temp buffer for decoding

    for (size_t i = 0; i < encoded.length(); ++i)
    {
        if (encoded[i] == '%')
        {
            // Check for valid encoding (two hex characters after %)
            if (i + 2 < encoded.length() &&
                isxdigit(encoded[i + 1]) &&
                isxdigit(encoded[i + 2]))
            {
                temp[0] = encoded[i + 1];
                temp[1] = encoded[i + 2];
                decoded += static_cast<char>(strtol(temp, nullptr, 16));
                i += 2; // Skip the next two hex characters
            }
            else
            {
                // Malformed encoding, append as-is
                decoded += encoded[i];
            }
        }
        else
        {
            decoded += encoded[i]; // Append non-encoded character
        }
    }

    return decoded;
}
std::string urlDecode(const std::string& encoded) {
    std::ostringstream decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hexValue = encoded.substr(i + 1, 2);
            if (isxdigit(hexValue[0]) && isxdigit(hexValue[1])) {
                int charCode;
                std::istringstream(hexValue) >> std::hex >> charCode;
                decoded << static_cast<char>(charCode);
                i += 2; // Skip next two characters
            } else {
                decoded << '%'; // Leave the '%' as-is if invalid
            }
        } else if (encoded[i] == '+') {
            decoded << ' ';
        } else {
            decoded << encoded[i];
        }
    }
    return decoded.str();
}