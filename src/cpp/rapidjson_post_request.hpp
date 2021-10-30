/*

Simple code written to send JSON POST requests using Beast for HTTP communication, and rapidjson for JSON
Only works on Windows if using my fork of boostorg/asio (unless my code has managed to be merged with Boost's)
Since my fork makes the single change of allowing "set_default_verify_paths" to work on Windows

BSD 3-Clause License

Copyright (c) 2021, Mason Hieb
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#pragma once
#include <cstdlib>
#include <iostream>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

inline void parse_string_into_res_body(http::response<http::string_body>& res,rapidjson::Document& res_body) {
    beast::string_view content_type = res[http::field::content_type];
    if (content_type.size() >= 16 && content_type.substr(0, 16) == "application/json") {
        res_body.Parse(res.body().c_str());
    }
}

void post_request_http(
        boost::asio::io_context& ioc,
        const boost::asio::ip::basic_resolver_results<tcp>& results,
        http::request<http::string_body>& req,
        rapidjson::Document& res_body
    ) {

        beast::tcp_stream stream(ioc);

        stream.connect(results);
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        parse_string_into_res_body(res,res_body);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};
    
}

void post_request_https(
    const std::string& host,
    boost::asio::io_context& ioc,
    const boost::asio::ip::basic_resolver_results<tcp>& results,
    http::request<http::string_body>& req,
    rapidjson::Document& res_body
) {

    ssl::context ctx(ssl::context::tlsv12_client);

    // This only works on Windows if using masonhieb/asio - set_default_verify_paths
    // modified to include custom Windows code to load Windows root certificates
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);

    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
    {
        beast::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
        throw beast::system_error{ ec };
    }

    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(ssl::stream_base::client);

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res);

    parse_string_into_res_body(res, res_body);

    beast::error_code ec;
    stream.shutdown(ec);

    if (ec == boost::asio::error::eof || ec == ssl::error::stream_truncated)
    {
        ec = {};
    }
    if (ec)
        throw beast::system_error{ ec };

}

void post_request(
    const rapidjson::Document& req_body,
    rapidjson::Document& res_body,
    const std::string& host,
    const std::string& port,
    const std::string& target,
    const std::string& user_agent,
    const bool https = true
) {

    http::request<http::string_body> req{ http::verb::post, target, 11 };
    req.set(http::field::host, host);
    req.set(http::field::user_agent, user_agent);
    req.set(http::field::connection, "keep-alive");
    req.set(http::field::content_type, "application/json; charset=UTF-8");

    if (req_body.IsNull()) {
        req.body() = "";
        req.prepare_payload();
    } else {
        rapidjson::StringBuffer strbuffer;
        strbuffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuffer);
        req_body.Accept(writer);
        req.body() = std::string(strbuffer.GetString());
        req.prepare_payload();
    }

    boost::asio::io_context ioc;

    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(host, port);

    if (https) {
        post_request_https(host, ioc, results, req, res_body);
    } else {
        post_request_http(ioc, results, req, res_body);
    }

}