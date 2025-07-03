#include "RequestHandler.h"
#include "Client.h"
#include "Session.h"
#include "utils.h"
#include "FileUtils.h"
#include <ctime>
#include <string>
#include <algorithm> // For std::replace
#include "BalancerClient.h"
#include "CoordinatorClient.h"
#include "EmailUtils.cc"

using namespace std;

HttpResponse RequestHandler::handle_home(const HttpRequest &request, Session &session)
{
    if (session.get_valid())
    {
        HttpResponse response(302, "", {}, "/menu");
        return response;
    }

    std::string page_name = "login";
    std::unordered_map<std::string, std::string> template_values;
    display_error(template_values, request);
    HttpResponse response(200, page_name, template_values);
    return response;
}

HttpResponse RequestHandler::handle_register(const HttpRequest &request, Session &session)
{

    if (session.get_valid())
    {
        HttpResponse response(302, "", {}, "/menu");
        return response;
    }

    // Form the response body
    std::string page_name = "register";
    std::unordered_map<std::string, std::string> template_values;

    if (request.get_method() == "GET")
    {
        display_error(template_values, request);
        return HttpResponse(200, page_name, template_values);
    }
    else
    {
        unordered_map<string, string> body_map = request.get_body_map();
        bool is_valid = register_validation(body_map, session);
        if (is_valid)
        {
            session.set_username(body_map["reg_username"]);
            session.set_session_id(generate_random_id());
            session.set_valid(true);
            HttpResponse response(302, "", template_values, "/menu");
            response.set_cookie(session.get_session_id());
            server.create_session(session.get_session_id(), session);

            // create the root folder for the user
            FileUtils file_utils(session);
            file_utils.setup_root_folder(session.get_username());

            return response;
        }
        else
        {
            template_values["<display_error>"] = "true";
            return HttpResponse(302, "", template_values, "/register?error=1");
        }
    }
}

bool RequestHandler::register_validation(unordered_map<string, string> token_map, Session &session)
{
    if (token_map.find("reg_username") == token_map.end() || token_map.find("reg_password") == token_map.end())
    {
        return false;
    }

    std::string username = token_map["reg_username"];
    std::string password = token_map["reg_password"];

    if (username.size() < 4 || password.size() < 3)
    {
        return false;
    }
    // set the server address here
    Client client(session);
    session.set_username(username);
    client.updateAddress();

    string stored_password;
    bool user_exist = client.Get(username + "#account", "password", stored_password);
    if (!stored_password.empty())
    {
        return false;
    }
    client.Put(username + "#account", "password", password);
    return true;
}

void RequestHandler::display_error(unordered_map<string, string> &template_values, const HttpRequest &request)
{
    bool display_error = request.get_query_map().find("error") != request.get_query_map().end();
    if (display_error)
    {
        template_values["<display_error>"] = "true";
    }
    else
    {
        template_values["<display_error>"] = "false";
    }
}

HttpResponse RequestHandler::handle_login(const HttpRequest &request, Session &session)
{

    if (session.get_valid())
    {
        HttpResponse response(302, "", {}, "/menu");
        return response;
    }

    unordered_map<string, string> token_map = request.get_body_map();

    bool is_valid = login_validation(token_map, session);
    std::unordered_map<std::string, std::string> template_values;

    if (is_valid)
    {
        HttpResponse response(302, "", template_values, "/menu");
        session.set_username(token_map["username"]);
        session.set_session_id(generate_random_id());
        response.set_cookie(session.get_session_id());
        server.create_session(session.get_session_id(), session);
        return response;
    }
    else
    {
        template_values["<display_error>"] = "true";
        return HttpResponse(302, "", template_values, "/?error=1");
    }
}

bool RequestHandler::login_validation(unordered_map<string, string> token_map, Session &session)
{
    if (token_map.find("username") == token_map.end() || token_map.find("password") == token_map.end())
    {
        return false;
    }

    std::string username = token_map["username"];
    std::string password = token_map["password"];
    Client client(session);
    session.set_username(username);
    client.updateAddress();

    string stored_password;
    bool user_exist = client.Get(username + "#account", "password", stored_password);
    if (!user_exist || stored_password != password)
    {
        return false;
    }

    return true;
}

HttpResponse RequestHandler::handle_menu(const HttpRequest &request)
{
    std::string page_name = "menu";
    std::unordered_map<std::string, std::string> template_values;
    HttpResponse response(200, page_name, template_values);
    return response;
}

HttpResponse RequestHandler::handle_storage(const HttpRequest &request, Session &session)
{
    std::string page_name = "storage";
    std::string folder = request.get_query_map().at("folder"); // get the folder id from the query (for root it's "root/")

    // Get the existing folder/file names from the storage
    FileUtils file_utils(session); 
    std::string username = session.get_username();

    string file_table = file_utils.generate_file_table(username, folder);
    std::unordered_map<std::string, std::string> template_values;
    template_values["<file_table>"] = file_table;
    template_values["<folder>"] = folder;
    template_values["<folder_tree>"] = file_utils.generate_folder_tree(username);
    HttpResponse response(200, page_name, template_values);
    return response;
}

HttpResponse RequestHandler::handle_inbox(const HttpRequest &request, Session &session)
{
    std::string page_name = "inbox";
    // Get the existing emails from the storage
    Client client(session);
    std::string username = session.get_username();
    std::string tab = "rcvd";

    // Create received emails table
    string list_of_emails;
    string row_name = username + "#email";
    cout << "row_name " << row_name << endl;

    bool email_list_exists = client.Get(row_name, "rcvdEmails", list_of_emails);
    string rcvd_email_table = "";
    cout << "email_list_exists " << email_list_exists << endl;
    if (email_list_exists)
    {
        rcvd_email_table = generate_email_table(list_of_emails, session, tab);
    }

    std::unordered_map<std::string, std::string> template_values;
    template_values["<rcvd_email_table>"] = rcvd_email_table;
    template_values["<username>"] = session.get_username();
    // cout << "template_values[<rcvd_email_table>]" << template_values["<rcvd_email_table>"] << endl;

    // Create sent emails table
    email_list_exists = client.Get(row_name, "sentEmails", list_of_emails);
    string sent_email_table = "";
    tab = "sent";
    cout << "email_list_exists " << email_list_exists << endl;
    if (email_list_exists)
    {
        sent_email_table = generate_email_table(list_of_emails, session, tab);
    }

    // std::unordered_map<std::string, std::string> template_values;
    template_values["<sent_email_table>"] = sent_email_table;
    template_values["<username>"] = session.get_username();
    // cout << "template_values[<sent_email_table>]" << template_values["<sent_email_table>"] << endl;

    HttpResponse response(200, page_name, template_values);
    return response;
}

string RequestHandler::generate_email_table(string list_of_emails, Session &session, string &tab)
{
    if (list_of_emails.empty())
    {
        return ""; // No emails to display
    }

    vector<string> email_ids;
    size_t pos = 0;
    while ((pos = list_of_emails.find(",")) != std::string::npos)
    {
        email_ids.push_back(list_of_emails.substr(0, pos));
        list_of_emails = list_of_emails.substr(pos + 1);
    }
    if (!list_of_emails.empty())
    {
        email_ids.push_back(list_of_emails); // Add the last email ID
    }

    string email_rows;
    for (const string &email_id : email_ids)
    {
        string row = generate_email_row(email_id, session, tab);
        if (!row.empty())
        {
            email_rows += row;
        }
    }
    return email_rows;
}

// Generates rows for the sent and received tables
string RequestHandler::generate_email_row(string email_id, Session &session, string &tab)
{
    Client client(session);
    string email_data;
    string row_name = session.get_username() + "#email";

    bool email_exist = client.Get(row_name, email_id + "#metadata", email_data);
    if (!email_exist)
    {
        return ""; // Email metadata not found
    }

    // Parse email_data: "timestamp#number_of_chunks#sender#recipient#"
    vector<string> parts;
    size_t pos = 0;
    while ((pos = email_data.find("#")) != std::string::npos)
    {
        parts.push_back(email_data.substr(0, pos));
        email_data = email_data.substr(pos + 1);
    }
    if (!email_data.empty())
    {
        parts.push_back(email_data);
    }

    if (parts.size() < 4)
    {
        return ""; // Malformed metadata
    }
    std::cout << "[TAB] " << tab.c_str() << std::endl;
    string timestamp = parts[0];
    string sender;
    string recipient;
    if (tab == "sent")
    {
        // std::cout << "tab is sent" << std::endl;
        sender = parts[3];
        recipient = parts[2];
    }
    else
    {
        // std::cout << "tab is rcvd" << std::endl;
        sender = parts[2];
        recipient = parts[3];
    }
    std::cout << "email_id: " << email_id << std::endl;
    std::cout << "sender: " << sender << std::endl;
    std::cout << "recipient: " << recipient << std::endl;
    std::string content;
    client.Get(row_name, email_id + "#data#1", content);
    std::string content_copy = content;
    pos = content_copy.find("Subject: ");
    content_copy = content_copy.substr(pos + 9);
    pos = content_copy.find("\r");
    std::string subject = content_copy.substr(0, pos);

    char html_row[1024];
    snprintf(
        html_row, sizeof(html_row),
        "<tr>"
        "<td>%s</td>" // Sender
        "<td>%s</td>"
        "<td>%s</td>" // Timestamp
        "<td>"
        "<div style=\"display: flex; align-items: center;\">"
        "<button class=\"ui tiny blue button\" onclick=\"view_email('%s')\">View Email</button>"
        "<button class=\"ui tiny red button\" onclick=\"delete_email('%s', '%s')\">Delete</button>"
        "<button class=\"ui tiny blue button\" onclick=\"forward_email('%s')\">Forward Email</button>"
        "<button class=\"ui tiny blue button\" onclick=\"reply_to_email('%s')\">Reply Email</button>"
        "</div>"
        "</td>"
        "</tr>",
        sender.c_str(), subject.c_str(), timestamp.c_str(),
        email_id.c_str(), email_id.c_str(), tab.c_str(),
        email_id.c_str(), email_id.c_str());

    return std::string(html_row);
}

// HttpResponse RequestHandler::serve_inbox(const HttpRequest& request, const string& username) {
//     HttpResponse response = handle_inbox(request, username);
//     // Replace {{{receivedEmails}}} in the response with generated email table
//     response.body.replace(response.body.find("{{{receivedEmails}}}"), 19, response.get_template_value("<email_table>"));
//     return response;
// }

HttpResponse RequestHandler::handle_upload_file(const HttpRequest &request, Session &session)
{

    FileUtils file_utils(session);
    std::string username = session.get_username();

    // Get the file name and data from the request
    File file = request.get_file();
    string folder = request.get_query_map().at("folder");
    string filename = file.name;
    string &data = file.data;
    file_utils.upload_file(username, filename, data, folder);

    // Redirect to the storage page
    return HttpResponse(302, "", {}, "/storage?folder=" + folder);
}

HttpResponse RequestHandler::handle_download(const HttpRequest &request, Session &session)
{

    FileUtils file_utils(session);
    std::string username = session.get_username();

    // Get the file name from the request
    std::string file_id = request.get_query_map().at("file");
    std::string file_data;
    file_utils.download_file(username, file_id, file_data);
    // Get the file name from the metadata
    std::string metadata = file_utils.get_file_metadata(username, file_id);
    std::unordered_map<std::string, std::string> metadata_map = file_utils.parse_file_metadata(metadata);
    std::string display_name = metadata_map["filename"];
    return HttpResponse(200, file_data, display_name);
}

std::string createEmailString(const std::string &from_email, const std::string &to_email, const std::string &subject, const std::string &body)
{
    std::string email;
    email += "HELO example.com\r\n";
    email += "MAIL FROM:<" + from_email + ">\r\n";
    email += "RCPT TO:<" + to_email + ">\r\n";
    email += "DATA\r\n";
    email += "Subject: " + subject + "\r\n";
    email += body + "\r\n";
    email += ".\r\n";
    return email;
}

std::string decodeURIComponent(const std::string &encoded)
{
    std::string decoded = encoded;
    size_t pos = 0;
    while ((pos = decoded.find("%40", pos)) != std::string::npos)
    {
        decoded.replace(pos, 3, "@");
        pos += 1;
    }
    return decoded;
}

bool validateEmailDomain(const std::string &to_email)
{
    const std::string required_domain = "@penncloud";
    std::string decoded_email = decodeURIComponent(to_email);
    if (decoded_email.size() >= required_domain.size() &&
        decoded_email.compare(decoded_email.size() - required_domain.size(), required_domain.size(), required_domain) == 0)
    {
        return true;
    }
    return false;
}

HttpResponse RequestHandler::handle_change_password(const HttpRequest &request, Session &session)
{

    // Present the form
    if (request.get_method() == "GET")
    {

        unordered_map<string, string> template_values;
        string page_name = "change_pwd";
        display_error(template_values, request);
        return HttpResponse(200, page_name, template_values, "");
    }
    else
    {
        // if method = POST - Handle the request to change the password after user submits the form
        string new_password = request.get_body_map().at("new_password");
        string current_password = request.get_body_map().at("current_password");
        Client client(session);
        std::string username = session.get_username();
        bool result = client.CPut(username + "#account", "password", current_password, new_password);
        // this section should be removed if CPut works correctly
        /**************/
        string stored_password;
        client.Get(username + "#account", "password", stored_password);
        if (stored_password != new_password)
        {
            result = false;
        }
        /**************/
        if (result)
        {
            return HttpResponse(302, "", {}, "/menu");
        }
        else
        {
            return HttpResponse(302, "", {}, "/change_password?error=1");
        }
    }
}

HttpResponse RequestHandler::handle_sendemail(const HttpRequest &request, Session &session)
{
    if (request.get_method() != "POST")
    {
        std::unordered_map<std::string, std::string> template_values = {};
        HttpResponse response(405, "Method Not Allowed", template_values);
        return response;
    }

    std::unordered_map<std::string, std::string> body_map = request.get_body_map();
    if (body_map.find("from") == body_map.end() || body_map.find("to") == body_map.end() ||
        body_map.find("subject") == body_map.end() || body_map.find("body") == body_map.end())
    {
        std::unordered_map<std::string, std::string> template_values = {};
        template_values["<email_sent>"] = "false";
        HttpResponse response(400, "Bad Request", template_values);
        return response;
    }

    std::string from_email = session.get_username() + "@penncloud";
    std::string to_email = decodeURIComponent(body_map["to"]);
    std::string subject = body_map["subject"];
    std::string body = body_map["body"];
    std::string address = server.get_server_id();
    send_email(from_email, to_email, subject, body, address);
    std::unordered_map<std::string, std::string> template_values;
    return HttpResponse(302, "", {}, "/inbox");
}

// HttpResponse RequestHandler::handle_forward_modal(const HttpRequest &request, Session &session) {
//     std::cout << "HANDLE FORWARD EMAIL " << std::endl;
//     std::string email_id;
//     try {
//         email_id = request.get_query_map().at("email_id");
//     } catch (const std::out_of_range &) {
//         return HttpResponse(400, "Bad Request", {{"<error_message>", "Missing email_id parameter"}});
//     }

//     std::string username = session.get_username();
//     std::string row_name = username + "#email";
//     std::cout << "row_name " << row_name << std::endl;

//     Client client(session);
//     std::string metadata;
//     if (!client.Get(row_name, email_id + "#metadata", metadata)) {
//         return HttpResponse(404, "Not Found", {{"<error_message>", "Email metadata not found"}});
//     }
//     std::cout << "got metadata for email ID " << email_id << std::endl;

//     // Parse metadata: "timestamp#number_of_chunks#sender#recipient#"
//     std::vector<std::string> parts;
//     size_t pos = 0;
//     while ((pos = metadata.find("#")) != std::string::npos) {
//         parts.push_back(metadata.substr(0, pos));
//         metadata = metadata.substr(pos + 1);
//     }
//     parts.push_back(metadata);

//     if (parts.size() < 4) {
//         return HttpResponse(400, "Bad Request", {{"<error_message>", "Malformed metadata"}});
//     }

//     int num_chunks = std::stoi(parts[1]);
//     std::cout << "num_chunks " << num_chunks << std::endl;

//     // Assemble the email content from chunks
//     std::string email_content;
//     for (int i = 1; i <= num_chunks; ++i) {
//         std::string chunk;
//         if (!client.Get(row_name, email_id + "#data#" + std::to_string(i), chunk)) {
//             return HttpResponse(500, "Internal Server Error", {{"<error_message>", "Failed to retrieve email chunks"}});
//         }
//         email_content += chunk;
//         std::cout << "email_content " << email_content << std::endl;
//     }

//     // Populate template values
//     std::unordered_map<std::string, std::string> template_values = {
//         {"<username>", username},
//         {"<subject>", "Fwd: " + parts[3]}, // Prefix subject with "Fwd:"
//         {"<body>", email_content}
//     };

//     // Render the forward email modal page
//     return HttpResponse(200, "forward_email", template_values);
// }

HttpResponse RequestHandler::handle_forward_modal(const HttpRequest &request, Session &session)
{
    // Check the request method
    if (request.get_method() == "GET")
    {
        // Retrieve the email data
        std::string email_id;
        try
        {
            email_id = request.get_query_map().at("email_id");
        }
        catch (const std::out_of_range &)
        {
            return HttpResponse(400, "Bad Request", {{"<error_message>", "Missing email_id parameter"}});
        }

        std::string username = session.get_username();
        std::string row_name = username + "#email";

        Client client(session);
        std::string metadata;
        if (!client.Get(row_name, email_id + "#metadata", metadata))
        {
            return HttpResponse(404, "Not Found", {{"<error_message>", "Email metadata not found"}});
        }

        // Parse the metadata
        std::vector<std::string> parts;
        {
            size_t pos = 0;
            std::string temp = metadata;
            while ((pos = temp.find("#")) != std::string::npos)
            {
                parts.push_back(temp.substr(0, pos));
                temp = temp.substr(pos + 1);
            }
            parts.push_back(temp);
        }

        if (parts.size() < 4)
        {
            return HttpResponse(400, "Bad Request", {{"<error_message>", "Malformed metadata"}});
        }

        int num_chunks = std::stoi(parts[1]);
        std::string original_sender = parts[3];

        // Assemble the email content from chunks
        std::string email_content;
        for (int i = 1; i <= num_chunks; ++i)
        {
            std::string chunk;
            if (!client.Get(row_name, email_id + "#data#" + std::to_string(i), chunk))
            {
                return HttpResponse(500, "Internal Server Error", {{"<error_message>", "Failed to retrieve email chunks"}});
            }
            email_content += chunk;
        }

        std::string content = email_content;
        int pos = content.find("Subject: ");
        content = content.substr(pos + 9);
        pos = content.find("\r");
        std::string subject = content.substr(0, pos);

        // Prepopulate the subject with "Fwd: "
        std::string forward_subject = "Fwd: " + subject;

        std::unordered_map<std::string, std::string> template_values = {
            {"<forward_subject>", forward_subject},
            {"<email_body>", email_content},
            {"<email_id>", email_id},
            {"<username>", username}};

        // Render a template that shows a forward form
        return HttpResponse(200, "forward_page", template_values);
    }
    else if (request.get_method() == "POST")
    {
        // Forward the email
        std::unordered_map<std::string, std::string> body_map = request.get_body_map();

        if (body_map.find("email_id") == body_map.end() || body_map.find("to") == body_map.end() || body_map.find("subject") == body_map.end() || body_map.find("body") == body_map.end())
        {
            return HttpResponse(400, "Bad Request", {{"<error_message>", "Missing required form parameters"}});
        }

        std::string email_id = body_map["email_id"];
        std::string to_email = decodeURIComponent(body_map["to"]);
        std::string subject = body_map["subject"];
        std::string email_content = body_map["body"];
        // std::string subject = urlDecode(body_map["subject"]);
        // std::string email_content = urlDecode(body_map["body"]);
        

        std::string from_email = session.get_username() + "@penncloud";
        std::string address = server.get_server_id();

        if (!send_email(from_email, to_email, subject, email_content, address))
        {
            return HttpResponse(500, "Internal Server Error", {{"<email_forwarded>", "false"}});
        }

        // Return a success message or redirect
        return HttpResponse(302, "", {{"<email_forwarded>", "true"}}, "/inbox");
    }
    else
    {
        // Any other method - return 405 Method Not Allowed
        return HttpResponse(405, "Method Not Allowed", std::unordered_map<std::string, std::string>(), "");
    }
}

HttpResponse RequestHandler::handle_reply_modal(const HttpRequest &request, Session &session)
{
    // Check the request method
    if (request.get_method() == "GET")
    {
        // Retrieve the email data
        std::string email_id;
        try
        {
            email_id = request.get_query_map().at("email_id");
        }
        catch (const std::out_of_range &)
        {
            return HttpResponse(400, "Bad Request", {{"<error_message>", "Missing email_id parameter"}});
        }

        std::string username = session.get_username();
        std::string row_name = username + "#email";

        Client client(session);
        std::string metadata;
        if (!client.Get(row_name, email_id + "#metadata", metadata))
        {
            return HttpResponse(404, "Not Found", {{"<error_message>", "Email metadata not found"}});
        }

        // Parse the metadata
        std::vector<std::string> parts;
        {
            size_t pos = 0;
            std::string temp = metadata;
            while ((pos = temp.find("#")) != std::string::npos)
            {
                parts.push_back(temp.substr(0, pos));
                temp = temp.substr(pos + 1);
            }
            parts.push_back(temp);
        }

        if (parts.size() < 4)
        {
            return HttpResponse(400, "Bad Request", {{"<error_message>", "Malformed metadata"}});
        }

        int num_chunks = std::stoi(parts[1]);
        std::string original_sender = parts[2];

        // Assemble the email content from chunks
        std::string email_content;
        for (int i = 1; i <= num_chunks; ++i)
        {
            std::string chunk;
            if (!client.Get(row_name, email_id + "#data#" + std::to_string(i), chunk))
            {
                return HttpResponse(500, "Internal Server Error", {{"<error_message>", "Failed to retrieve email chunks"}});
            }
            email_content += chunk;
        }

        std::string content_copy = email_content;
        int pos = content_copy.find("Subject: ");
        content_copy = content_copy.substr(pos + 9);
        pos = content_copy.find("\r");
        std::string subject = content_copy.substr(0, pos);

        // Prepopulate the subject with "Fwd: "
        std::string reply_subject = "Re: " + subject;

        std::unordered_map<std::string, std::string> template_values = {
            {"<username>", username},
            {"<recipient>", original_sender},   // The original sender becomes the recipient for the reply
            {"<reply_subject>", reply_subject}, // Prefix subject with "Re:"
            {"<email_body>", email_content},
            {"<email_id>", email_id}};

        // Render a template that shows a forward form
        return HttpResponse(200, "reply_page", template_values);
    }
    else if (request.get_method() == "POST")
    {
        // Send the email
        std::unordered_map<std::string, std::string> body_map = request.get_body_map();

        if (body_map.find("email_id") == body_map.end() || body_map.find("to") == body_map.end() || body_map.find("subject") == body_map.end() || body_map.find("body") == body_map.end())
        {
            return HttpResponse(400, "Bad Request", {{"<error_message>", "Missing required form parameters"}});
        }

        std::string email_id = body_map["email_id"];
        std::string to_email = decodeURIComponent(body_map["to"]);
        std::string subject = body_map["subject"];
        std::string email_content = body_map["email_body"];
        // std::string subject = urlDecode(body_map["subject"]);
        // std::string email_content = urlDecode(body_map["email_body"]);

        std::string from_email = session.get_username() + "@penncloud";
        std::string address = server.get_server_id();

        if (!send_email(from_email, to_email, subject, email_content, address))
        {
            return HttpResponse(500, "Internal Server Error", {{"<email_replied>", "false"}});
        }

        // Return a success message or redirect
        return HttpResponse(302, "", {{"<email_replied>", "true"}}, "/inbox");
    }
    else
    {
        // Any other method - return 405 Method Not Allowed
        return HttpResponse(405, "Method Not Allowed", std::unordered_map<std::string, std::string>(), "");
    }
}

HttpResponse RequestHandler::handle_logout(const HttpRequest &request, Session &session)
{
    server.remove_session(session.get_session_id());
    session.clear();
    HttpResponse response(302, "", {}, "/");
    response.delete_cookie();
    return response;
}

unordered_map<string, string> RequestHandler::get_fe_status()
{

    BalancerClient client;
    string status = client.get_status("");
    // parse the status
    // format 5015#status#5016#status#5017#status#5018#status#
    // format - <fe_status1>, Running/Down
    unordered_map<string, string> status_map;
    while (!status.empty())
    {
        int pos = status.find("#");
        string key = status.substr(0, pos);
        status = status.substr(pos + 1);
        pos = status.find("#");
        string value = status.substr(0, pos);
        status = status.substr(pos + 1);
        status_map["<" + key + "_status" + ">"] = value;
    }
    return status_map;
}

unordered_map<string, string> RequestHandler::get_kvs_status()
{

    CoordinatorClient client;
    std::map<std::string, bool> status_map = client.getAllServerStatus();

    unordered_map<string, string> kvs_status;
    for (const auto &entry : status_map)
    {
        std::string status = entry.second ? "Running" : "Down";
        kvs_status["<" + entry.first + "_status" + ">"] = status;
    }
    return kvs_status;
    // get the status of the KVS
}

HttpResponse RequestHandler::handle_admin(const HttpRequest &request)
{
    std::string page_name = "admin";
    // get the status from load balancer
    std::unordered_map<std::string, std::string> template_values = get_fe_status();
    std::unordered_map<std::string, std::string> kvs_status = get_kvs_status();
    template_values.insert(kvs_status.begin(), kvs_status.end());
    HttpResponse response(200, page_name, template_values);
    return response;
}

HttpResponse RequestHandler::handle_change_status(const HttpRequest &request)
{

    std::string node = request.get_query_map().at("node");
    std::string status = request.get_query_map().at("status");
    Session session;
    session.set_address_port(node);
    Client client(session);
    if (status == "start")
    {
        client.Revive();
    }
    else
    {
        Client client(session);
        client.Kill();
    }
    return HttpResponse(302, "", {}, "/admin");
}

HttpResponse RequestHandler::handle_create_folder(const HttpRequest &request, Session &session)
{
    std::string folder_name = request.get_body_map().at("folder_name");
    std::string folder = request.get_query_map().at("folder");
    std::string username = session.get_username();
    FileUtils file_utils(session);
    file_utils.create_folder(username, folder_name, folder);
    return HttpResponse(302, "", {}, "/storage?folder=" + folder);
}

HttpResponse RequestHandler::handle_delete_file(const HttpRequest &request, Session &session)
{
    std::string file_id = request.get_query_map().at("file");
    std::string username = session.get_username();
    FileUtils file_utils(session);

    std::string metadata = file_utils.get_file_metadata(username, file_id);
    std::unordered_map<std::string, std::string> metadata_map = file_utils.parse_file_metadata(metadata);
    std::string folder_id = metadata_map["parent_folder_id"];
    file_utils.delete_file(username, file_id);
    return HttpResponse(302, "", {}, "/storage?folder=" + folder_id);
}

HttpResponse RequestHandler::handle_delete_folder(const HttpRequest &request, Session &session)
{
    std::string folder_id = request.get_query_map().at("folder");
    std::string username = session.get_username();
    FileUtils file_utils(session);
std:;
    string metadata = file_utils.get_folder_metadata(username, folder_id);
    std::unordered_map<std::string, std::string> metadata_map = file_utils.parse_folder_metadata(metadata);
    std::string parent_folder_id = metadata_map["parent_folder_id"];
    file_utils.delete_folder(username, folder_id);
    return HttpResponse(302, "", {}, "/storage?folder=" + parent_folder_id);
}

// old filename is the full path while new filename is just the name!
HttpResponse RequestHandler::handle_rename(const HttpRequest &request, Session &session)
{
    std::string file_id = url_decode(request.get_body_map().at("old_filename"));
    std::string new_filename = url_decode(request.get_body_map().at("new_filename"));
    std::string username = session.get_username();
    FileUtils file_utils(session);
    std::string metadata = file_utils.get_file_metadata(username, file_id);
    bool is_folder = false;
    if (metadata.empty())
    {
        is_folder = true;
        metadata = file_utils.get_folder_metadata(username, file_id);
        file_utils.rename_folder(username, file_id, new_filename);
    }
    else
    {
        file_utils.rename_file(username, file_id, new_filename);
    }
    std::string parent_id = metadata.substr(0, metadata.find("#"));
    return HttpResponse(302, "", {}, "/storage?folder=" + parent_id);
}

HttpResponse RequestHandler::handle_move(const HttpRequest &request, Session &session)
{
    std::string file_id = request.get_body_map().at("item_id");
    std::string destination_folder = request.get_body_map().at("destination_folder");
    destination_folder = url_decode(destination_folder);
    FileUtils file_utils(session);
    std::string username = session.get_username();
    std::string metadata = file_utils.get_file_metadata(username, file_id);
    if (metadata.empty())
    {
        metadata = file_utils.get_folder_metadata(username, file_id);
        file_utils.move_folder(username, file_id, destination_folder);
    }
    else
    {
        file_utils.move_file(username, file_id, destination_folder);
    }
    return HttpResponse(302, "", {}, "/storage?folder=" + destination_folder);
}

std::string RequestHandler::generate_raw_data_value(std::string node, std::string key, std::string column)
{
    // just return the value directly
    if (column.find("#data#") == std::string::npos)
    {
        // get the value from kvs
        Session session;
        session.set_address_port(node);
        Client client(session);
        std::string value;
        client.Get(key, column, value);
        return value;
    }
    else
    {
        // return the href to view the file
        std::string encoded_key = key;
        replace_all(encoded_key, "#", "%23");
        replace_all(encoded_key, "@", "%40");
        std::string encoded_column = column;
        replace_all(encoded_column, "#", "%23");
        replace_all(encoded_column, "@", "%40");
        return "<a href=\"/get_data?node=" + node + "&key=" + encoded_key + "&column=" + encoded_column + "\">View content</a>";
    }
}

std::string RequestHandler::generate_raw_data_table(std::string node)
{
    Session session;
    session.set_address_port(node);
    Client client(session);
    std::map<std::string, std::vector<std::string>> raw_data;
    std::cout << "Fetching keys from: " << session.get_address_port() << std::endl;
    bool result = client.FetchKeys(raw_data);
    if (!result)
    {
        return "";
    }

    std::string table;
    for (const auto &entry : raw_data)
    {
        std::string encoded_key = entry.first;
        replace_all(encoded_key, "#", "%23");
        replace_all(encoded_key, "@", "%40");

        for (const auto &column : entry.second)
        {
            table += "<tr>";
            table += "<td>" + entry.first + "</td>";
            table += "<td>";
            table += column + "</td>";
            table += "<td>";
            table += generate_raw_data_value(node, entry.first, column);
            table += "</tr>";
        }
    }
    return table;
}

HttpResponse RequestHandler::handle_raw_data(const HttpRequest &request)
{
    std::string node = request.get_query_map().at("node");
    std::string page_name = "raw_data";
    std::unordered_map<std::string, std::string> template_values;
    template_values["<raw_data>"] = generate_raw_data_table(node);
    HttpResponse response(200, page_name, template_values);
    return response;
}

HttpResponse RequestHandler::handle_view_email(const HttpRequest &request, Session &session)
{
    std::cout << "in handle view email" << std::endl;
    std::string email_id = request.get_query_map().at("email_id");
    std::string username = session.get_username();
    Client client(session);
    std::string metadata;
    client.Get(username + "#email", email_id + "#metadata", metadata);
    // get the email data here
    unordered_map<string, string> email_map = parse_email_metadata(metadata);
    std::unordered_map<std::string, std::string> template_values;
    // parse from the email metadata?
    // not sure how to get the below stuff?
    std::string email_content;
    client.Get(username + "#email", email_id + "#data#1", email_content);
    std::string content_copy = email_content;
    int pos = content_copy.find("Subject: ");
    content_copy = content_copy.substr(pos + 9);
    pos = content_copy.find("\r");
    std::string subject = content_copy.substr(0, pos);
    template_values["<email_content>"] = email_content;
    template_values["<subject>"] = subject;
    template_values["<email_id>"] = email_id;
    template_values["<sender>"] = email_map["sender"];
    template_values["<recipient>"] = email_map["recipient"];
    template_values["<timestamp>"] = email_map["timestamp"];
    HttpResponse response(200, "view_email", template_values);
    return response;
}

HttpResponse RequestHandler::handle_delete_email(const HttpRequest &request, Session &session, string &tab)
{
    // std::cout << "in handle delete email" << std::endl;
    // Get the username from the session
    std::string username = session.get_username();
    std::cout << "username " << username << std::endl;

    // Construct the row name
    std::string row_name = username + "#email";
    std::cout << "row_name " << row_name << std::endl;

    // Use the column name rcvdEmails to get the list of received emails
    Client client(session);
    std::string emails;
    std::string col_name = tab + "Emails";
    std::cout << "tab " << tab << std::endl;
    std::cout << "col_name " << col_name << std::endl;
    if (!client.Get(row_name, col_name, emails))
    {
        std::cout << "nothing in Get " << std::endl;
        return HttpResponse(404, "error", {{"<error_message>", "No received emails found"}});
    }
    std::cout << "value in Get " << std::endl;

    // Get the email ID from the request query map
    std::string email_id;

    try
    {
        email_id = request.get_query_map().at("email_id");
        std::cout << "email_id " << email_id << std::endl;
    }
    catch (const std::out_of_range &)
    {
        return HttpResponse(400, "error", {{"<error_message>", "Missing email_id parameter in request"}});
    }

    // Remove the email ID from the list of received emails
    std::vector<std::string> email_ids;
    size_t pos = 0;

    // Split emails into a vector of email IDs
    while ((pos = emails.find(",")) != std::string::npos)
    {
        std::string id = emails.substr(0, pos);
        emails.erase(0, pos + 1);
        if (id != email_id)
        {
            email_ids.push_back(id); // Keep all email IDs except the one to be deleted
        }
    }
    if (!emails.empty() && emails != email_id)
    {
        email_ids.push_back(emails); // Add the last email ID if it's not the one being deleted
    }

    // Rebuild the updated list of emails as a comma-separated string
    std::string updated_email_list;
    for (size_t i = 0; i < email_ids.size(); ++i)
    {
        updated_email_list += email_ids[i];
        if (i != email_ids.size() - 1)
        {
            updated_email_list += ",";
        }
    }

    // Put the updated list back in the KVS
    if (!client.Put(row_name, col_name, updated_email_list))
    {
        return HttpResponse(500, "error", {{"<error_message>", "Failed to update received emails"}});
    }
    std::cout << "Put call successful " << std::endl;
    // Return a success response
    return HttpResponse(302, "", {}, "/inbox");
}

HttpResponse RequestHandler::handle_get_data(const HttpRequest &request)
{
    std::string node = request.get_query_map().at("node");
    std::string key = request.get_query_map().at("key");
    std::string column = request.get_query_map().at("column");

    replace_all(key, "%40", "@");
    replace_all(key, "%23", "#");
    replace_all(column, "%40", "@");
    replace_all(column, "%23", "#");

    std::string value;
    Session session;
    session.set_address_port(node);
    Client client(session);
    bool result = client.Get(key, column, value);
    if (!result)
    {
        std::cout << "Error: Could not fetch data from KVS" << std::endl;
        return HttpResponse(302, "", {}, "/admin");
    }
    else
    {
        unordered_map<string, string> template_values;
        template_values["<node>"] = node;
        template_values["<value>"] = value;
        return HttpResponse(200, "view_data", template_values);
    }
}

HttpResponse RequestHandler::handle_request(const HttpRequest &request, Session &session)
{
    std::string method = request.get_method();
    std::string uri = request.get_uri();

    if (uri == "/")
    {
        return handle_home(request, session);
    }
    else if (uri == "/register")
    {
        return handle_register(request, session);
    }
    else if (uri == "/login")
    {
        return handle_login(request, session);
    }
    else if (uri == "/menu")
    {
        return handle_menu(request);
    }
    else if (uri == "/storage")
    {
        return handle_storage(request, session);
    }
    else if (uri == "/upload_file")
    {
        return handle_upload_file(request, session);
    }
    else if (uri == "/download")
    {
        return handle_download(request, session);
    }
    else if (uri == "/inbox")
    {
        return handle_inbox(request, session);
    }
    else if (uri == "/send-email")
    {
        return handle_sendemail(request, session);
    }
    else if (uri == "/forward-email")
    {
        return handle_forward_modal(request, session);
    }
    else if (uri == "/change_password")
    {
        return handle_change_password(request, session);
    }
    else if (uri == "/logout")
    {
        return handle_logout(request, session);
    }
    else if (uri == "/admin")
    {
        return handle_admin(request);
    }
    else if (uri == "/create_folder")
    {
        return handle_create_folder(request, session);
    }
    else if (uri == "/delete_file")
    {
        return handle_delete_file(request, session);
    }
    else if (uri == "/delete_folder")
    {
        return handle_delete_folder(request, session);
    }
    else if (uri == "/rename")
    {
        return handle_rename(request, session);
    }
    else if (uri == "/move")
    {
        return handle_move(request, session);
    }
    else if (uri == "/change_status")
    {
        return handle_change_status(request);
    }
    else if (uri == "/raw_data")
    {
        return handle_raw_data(request);
    }
    else if (uri == "/view_email")
    {
        return handle_view_email(request, session);
    }
    else if (uri == "/reply-to-email")
    {
        return handle_reply_modal(request, session);
    }
    else if (uri == "/delete_email")
    {
        std::string tab; // Declare the `tab` variable
        try
        {
            tab = decodeTabComponent(request.get_query_map().at("tab")); // Initialize it with the `tab` parameter from the request query map
            std::cout << "tab" << tab << std::endl;
        }
        catch (const std::out_of_range &)
        {
            tab = "rcvd"; // Fallback to a default value, e.g., "rcvd"
        }
        return handle_delete_email(request, session, tab);
    }
    else if (uri == "/forward-email")
    {
        return handle_forward_modal(request, session);
    }
    else if (uri == "/get_data")
    {
        return handle_get_data(request);
    }

    // Return a 404 error for unhandled routes
    std::unordered_map<std::string, std::string> template_values = {};
    HttpResponse response(404, "", template_values, "");
    return response;
}