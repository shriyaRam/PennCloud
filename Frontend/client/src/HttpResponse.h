#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "HttpRequest.h"
#include <string>

class HttpResponse {
    
    private:
        int status_code;
        std::string response_header;
        std::string response_body;
        std::string redirect_location;

    public:

        HttpResponse(int status_code, const std::string& page_name, 
            const std::unordered_map<std::string, std::string>& template_values = {}, string redirect="");

        // this constructor is for downloading files only
        HttpResponse(int status_code, string file_data, string filename);
        
        void form_response_body(const std::string& page_name, const std::unordered_map<std::string, std::string>& template_values);

        void form_response_header(int body_length, bool close);
        std::string read_webpage(std::string page_key);
        std::string get_response() { return response_header + response_body; }
        std::string get_response_header() { return response_header; }

        // cookie
        void set_cookie(std::string session_id);
        void delete_cookie();

        void set_redirect_location(const std::string& location) {redirect_location = location;}
};

#endif // HTTPRESPONSE_H
