#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "HttpRequest.h"
#include <string>

class HttpResponse {
    
    private:
        int status_code; // Stores the status code of the response
        std::string response_header; // Stores the response header
        std::string redirect_location; // Stores the redirect location for 302

    public:

        HttpResponse(int status_code, const std::string& page_name, 
            const std::unordered_map<std::string, std::string>& template_values = {}, string redirect="");
        // Form the response body
        void form_response_body(const std::string& page_name, const std::unordered_map<std::string, std::string>& template_values);
        // Form the response header
        void form_response_header(int body_length, bool close);
        // Get the response (header + body)
        std::string get_response() { return response_header;}
        // Set the redirect location for 302
        void set_redirect_location(const std::string& location) {redirect_location = location;}
};

#endif // HTTPRESPONSE_H
