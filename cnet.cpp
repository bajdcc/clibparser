//
// Project: clibparser
// Created by bajdcc
//

#include <asio.hpp>
#include "cnet.h"

#define LOG_NET 1

namespace clib {

    string_t cnet::http_get(const string_t &url) {
        size_t index;

        auto URL = url;

        static const char *http_protocol[] = {"http://", "https://"};
        auto is_http = false;
        if (url.find("/http/") != string_t::npos) {
            URL = url.substr(6);
        } else if (url.find("/https/") != string_t::npos) {
                is_http = true;
            URL = url.substr(7);
        }

#if LOG_NET
        printf("[SYSTEM] NET  | GET: %s%s\n", http_protocol[is_http], URL.c_str());
#endif

        index = URL.find('/');
        auto host = index == string_t::npos ? URL : URL.substr(0, index);
        auto path = index == string_t::npos ? "/" : URL.substr(index, url.length() - index);

        asio::io_service io_service;

        asio::ip::tcp::resolver resolver(io_service);
        asio::ip::tcp::resolver::query query(host, "http");
        asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

        asio::ip::tcp::socket socket(io_service);
        asio::connect(socket, iter);

        asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << "GET " << path << " HTTP/1.1\r\n";
        request_stream << "Host: " << host << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        asio::write(socket, request);

        asio::streambuf response;
        asio::read_until(socket, response, "\r\n");

        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
#if LOG_NET
            printf("[SYSTEM] NET  | GET: %s%s, #ERROR: Invalid response\n", http_protocol[is_http], URL.c_str());
#endif
        }
        if (status_code != 200) {
#if LOG_NET
            printf("[SYSTEM] NET  | GET: %s%s, #ERROR: Invalid status code= %d\n", http_protocol[is_http], URL.c_str(), status_code);
#endif
        }

        asio::read_until(socket, response, "\r\n\r\n");

        std::string header;
        int len = 0;
        while (getline(response_stream, header) && header != "\r") {
            if (header.find("Content-Length: ") == 0) {
                std::stringstream stream;
                stream << header.substr(16);
                stream >> len;
            }
        }

        auto size = response.size();

        asio::error_code error;

        while (asio::read(socket, response, asio::transfer_at_least(1), error)) {
            size = response.size();
        }

        if (error != asio::error::eof) {
            throw asio::system_error(error);
        }

        std::istream is(&response);
        is.unsetf(std::ios_base::skipws);
        std::string sz;
        sz.append(std::istream_iterator<char>(is), std::istream_iterator<char>());

#if LOG_NET
        printf("[SYSTEM] NET  | GET: %s%s, #SUCCESS: size1= %d, size2= %d\n", http_protocol[is_http], URL.c_str(), size, sz.length());
#endif

        return sz;
    }
}