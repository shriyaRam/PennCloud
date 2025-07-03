#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <string>
#include <unordered_map>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Session.h"
#include "FileUtils.h"
#include "FrontendServer.h"

class RequestHandler
{

public:
    // Main function to handle the request and return the response

    HttpResponse handle_request(const HttpRequest &request, Session &session);
    RequestHandler(FrontendServer &server) : server(server) {}

private:
    // Frontend server -> for session management
    FrontendServer &server;

    HttpResponse handle_home(const HttpRequest &request, Session &session);
    HttpResponse handle_register(const HttpRequest &request, Session &session);
    bool register_validation(unordered_map<string, string> token_map, Session &session);

    HttpResponse handle_login(const HttpRequest &request, Session &session);
    bool login_validation(unordered_map<string, string> token_map, Session &session);
    void display_error(unordered_map<string, string> &template_values, const HttpRequest &request);

    HttpResponse handle_menu(const HttpRequest &request);

    HttpResponse handle_storage(const HttpRequest &request, Session &session);
    HttpResponse handle_upload_file(const HttpRequest &request, Session &session);

    HttpResponse handle_download(const HttpRequest &request, Session &session);
    HttpResponse handle_create_folder(const HttpRequest &request, Session &session);
    HttpResponse handle_delete_file(const HttpRequest &request, Session &session);
    HttpResponse handle_delete_folder(const HttpRequest &request, Session &session);
    HttpResponse handle_rename(const HttpRequest &request, Session &session);
    HttpResponse handle_move(const HttpRequest &request, Session &session);

    HttpResponse handle_inbox(const HttpRequest &request, Session &session);
    HttpResponse serve_inbox(const HttpRequest &request, const string &username);
    string generate_email_table(string list_of_emails, Session &session, std::string &tab);
    string generate_email_row(string email_id, Session &session, std::string &tab);
    HttpResponse handle_sendemail(const HttpRequest &request, Session &session);
    HttpResponse handle_view_email(const HttpRequest &request, Session &session);
    HttpResponse handle_delete_email(const HttpRequest &request, Session &session, std::string &tab);
    HttpResponse handle_forward_modal(const HttpRequest &request, Session &session);
    HttpResponse handle_reply_modal(const HttpRequest &request, Session &session);

    HttpResponse handle_change_password(const HttpRequest &request, Session &session);
    HttpResponse handle_logout(const HttpRequest &request, Session &session);

    HttpResponse handle_admin(const HttpRequest &request);
    unordered_map<string, string> get_fe_status();
    unordered_map<string, string> get_kvs_status();

    HttpResponse handle_change_status(const HttpRequest &request);
    HttpResponse handle_raw_data(const HttpRequest &request);
    std::string generate_raw_data_table(std::string node);
    HttpResponse handle_get_data(const HttpRequest &request);
    std::string generate_raw_data_value(std::string node, std::string key, std::string column);
};

#endif // REQUESTHANDLER_H
