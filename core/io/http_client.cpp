/*************************************************************************/
/*  http_client.cpp                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "http_client.h"

#include "core/io/stream_peer.h"
#include "core/io/stream_peer_ssl.h"
#include "core/io/stream_peer_tcp.h"
#include "core/method_bind.h"
#include "core/os/os.h"
#include "core/version.h"
#include "core/version_generated.gen.h"
#include "core/string_formatter.h"

const char *HTTPClient::_methods[METHOD_MAX] = { "GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "TRACE", "CONNECT",
                                                 "PATCH" };

VARIANT_ENUM_CAST(HTTPClient::ResponseCode);
VARIANT_ENUM_CAST(HTTPClient::Method);
VARIANT_ENUM_CAST(HTTPClient::Status);

IMPL_GDCLASS(HTTPClient)

#ifndef JAVASCRIPT_ENABLED
Error HTTPClient::connect_to_host(const String &p_host, int p_port, bool p_ssl, bool p_verify_host) {
    close();

    conn_port = p_port;
    conn_host = p_host;

    ip_candidates.clear();

    ssl = p_ssl;
    ssl_verify_host = p_verify_host;

    String host_lower = StringUtils::to_lower(conn_host);
    if (StringUtils::begins_with(host_lower, "http://")) {
        conn_host = StringUtils::substr(conn_host, 7, conn_host.length() - 7);
    } else if (StringUtils::begins_with(host_lower, "https://")) {
        ssl = true;
        conn_host = StringUtils::substr(conn_host, 8, conn_host.length() - 8);
    }

    ERR_FAIL_COND_V(conn_host.length() < HOST_MIN_LEN, ERR_INVALID_PARAMETER);

    if (conn_port < 0) {
        if (ssl) {
            conn_port = PORT_HTTPS;
        } else {
            conn_port = PORT_HTTP;
        }
    }

    connection = tcp_connection;

    if (ssl && https_proxy_port != -1) {
        proxy_client = make_ref_counted<HTTPClient>();
        server_host = https_proxy_host;
        server_port = https_proxy_port;
    } else if (!ssl && http_proxy_port != -1) {
        server_host = http_proxy_host;
        server_port = http_proxy_port;
    } else {
        server_host = conn_host;
        server_port = conn_port;
    }

    if (StringUtils::is_valid_ip_address(conn_host)) {
        // Host contains valid IP
        Error err = tcp_connection->connect_to_host(IP_Address(server_host), server_port);
        if (err) {
            status = STATUS_CANT_CONNECT;
            return err;
        }

        status = STATUS_CONNECTING;
    } else {
        // Host contains hostname and needs to be resolved to IP
        resolving = IP::get_singleton()->resolve_hostname_queue_item(server_host);
        if (resolving == IP::RESOLVER_INVALID_ID) {
            status = STATUS_CANT_RESOLVE;
            return ERR_CANT_RESOLVE;
        }
        status = STATUS_RESOLVING;
    }

    return OK;
}

void HTTPClient::set_connection(const Ref<StreamPeer> &p_connection) {
    ERR_FAIL_COND_MSG(not p_connection, "Connection is not a reference to a valid StreamPeer object.");

    if (ssl) {
        ERR_FAIL_NULL_MSG(object_cast<StreamPeerSSL>(p_connection.get()),
                "Connection is not a reference to a valid StreamPeerSSL object.");
    }

    if (connection == p_connection) {
        return;
    }

    close();
    connection = p_connection;
    status = STATUS_CONNECTED;
}

Ref<StreamPeer> HTTPClient::get_connection() const {
    return connection;
}

static bool _check_request_url(HTTPClient::Method p_method, StringView p_url) {
    switch (p_method) {
        case HTTPClient::METHOD_CONNECT: {
            // Authority in host:port format, as in RFC7231
            auto pos = p_url.find(':');
            return pos != StringView::npos && pos < p_url.length() - 1;
        }
        case HTTPClient::METHOD_OPTIONS: {
            if (p_url == "*") {
                return true;
            }

            [[fallthrough]];
        }
        default:
            // Absolute path or absolute URL
            return p_url.starts_with("/") || p_url.starts_with("http://") || p_url.starts_with("https://");
    }
}

Error HTTPClient::request_raw(
        Method p_method, StringView p_url, const Vector<String> &p_headers, const Vector<uint8_t> &p_body) {
    ERR_FAIL_INDEX_V(p_method, METHOD_MAX, ERR_INVALID_PARAMETER);
    ERR_FAIL_COND_V(!_check_request_url(p_method, p_url), ERR_INVALID_PARAMETER);
    ERR_FAIL_COND_V(status != STATUS_CONNECTED, ERR_INVALID_PARAMETER);
    ERR_FAIL_COND_V(not connection, ERR_INVALID_DATA);

    String uri(p_url);
    if (!ssl && http_proxy_port != -1) {
        uri = FormatVE("http://%.*s:%d%.*s", (int)conn_host.size(),conn_host.data(), conn_port, (int)p_url.size(),p_url.data());
    }

    String request = String(_methods[p_method]) + " " + uri + " HTTP/1.1\r\n";

    bool add_host = true;
    bool add_clen = p_body.size() > 0;
    bool add_uagent = true;
    bool add_accept = true;
    for (const String &hdr : p_headers) {
        request += hdr + "\r\n";
        if (add_host && StringUtils::findn(hdr, "Host:") == 0) {
            add_host = false;
        }
        if (add_clen && StringUtils::findn(hdr, "Content-Length:") == 0) {
            add_clen = false;
        }
        if (add_uagent && StringUtils::findn(hdr, "User-Agent:") == 0) {
            add_uagent = false;
        }
        if (add_accept && StringUtils::findn(hdr, "Accept:") == 0) {
            add_accept = false;
        }
    }
    if (add_host) {
        if ((ssl && conn_port == PORT_HTTPS) || (!ssl && conn_port == PORT_HTTP)) {
            // Don't append the standard ports
            request += "Host: " + conn_host + "\r\n";
        } else {
            request += "Host: " + conn_host + ":" + itos(conn_port) + "\r\n";
        }
    }
    if (add_clen) {
        request += "Content-Length: " + ::to_string(p_body.size()) + "\r\n";
        // Should it add utf8 encoding?
    }
    if (add_uagent) {
        request += "User-Agent: GodotEngine/" + String(VERSION_FULL_BUILD) + " (" + OS::get_singleton()->get_name() +
                   ")\r\n";
    }
    if (add_accept) {
        request += ("Accept: */*\r\n");
    }
    request += ("\r\n");

    // TODO: SEGS: why on earth are there allocations made here, when it could just call connection->put_data a few
    // times?
    Vector<uint8_t> data;
    data.reserve(request.length() + p_body.size());
    data.resize(request.length());
    memcpy(data.data(), request.data(), request.size());

    data.push_back(p_body);

    Error err = connection->put_data(&data[0], data.size());

    if (err) {
        close();
        status = STATUS_CONNECTION_ERROR;
        return err;
    }

    status = STATUS_REQUESTING;
    head_request = p_method == METHOD_HEAD;

    return OK;
}

Error HTTPClient::request(Method p_method, StringView p_url, const Vector<String> &p_headers, const String &p_body) {
    ERR_FAIL_INDEX_V(p_method, METHOD_MAX, ERR_INVALID_PARAMETER);
    ERR_FAIL_COND_V(!_check_request_url(p_method, p_url), ERR_INVALID_PARAMETER);
    ERR_FAIL_COND_V(status != STATUS_CONNECTED, ERR_INVALID_PARAMETER);
    ERR_FAIL_COND_V(not connection, ERR_INVALID_DATA);

    String uri(p_url);
    if (!ssl && http_proxy_port != -1) {
        uri = FormatVE("http://%.*s:%d%.*s", (int)conn_host.size(),conn_host.data(), conn_port, (int)p_url.size(),p_url.data());
    }

    String request = String(_methods[p_method]) + " " + uri + " HTTP/1.1\r\n";

    bool add_host = true;
    bool add_uagent = true;
    bool add_accept = true;
    bool add_clen = p_body.length() > 0;
    for (const String &hdr : p_headers) {
        request += hdr + "\r\n";
        if (add_host && StringUtils::findn(hdr, "Host:") == 0) {
            add_host = false;
        }
        if (add_clen && StringUtils::findn(hdr, ("Content-Length:")) == 0) {
            add_clen = false;
        }
        if (add_uagent && StringUtils::findn(hdr, ("User-Agent:")) == 0) {
            add_uagent = false;
        }
        if (add_accept && StringUtils::findn(hdr, ("Accept:")) == 0) {
            add_accept = false;
        }
    }
    if (add_host) {
        if ((ssl && conn_port == PORT_HTTPS) || (!ssl && conn_port == PORT_HTTP)) {
            // Don't append the standard ports
            request += "Host: " + conn_host + "\r\n";
        } else {
            request += "Host: " + conn_host + ":" + itos(conn_port) + "\r\n";
        }
    }
    if (add_clen) {
        request += "Content-Length: " + ::to_string(p_body.length()) + "\r\n";
        // Should it add utf8 encoding?
    }
    if (add_uagent) {
        request += "User-Agent: GodotEngine/" + String(VERSION_FULL_BUILD) + " (" + OS::get_singleton()->get_name() +
                   ")\r\n";
    }
    if (add_accept) {
        request += ("Accept: */*\r\n");
    }
    request += ("\r\n");
    request += p_body;

    Error err = connection->put_data((const uint8_t *)request.data(), request.length());
    if (err) {
        close();
        status = STATUS_CONNECTION_ERROR;
        return err;
    }

    status = STATUS_REQUESTING;
    head_request = p_method == METHOD_HEAD;

    return OK;
}

bool HTTPClient::has_response() const {
    return !response_headers.empty();
}

bool HTTPClient::is_response_chunked() const {
    return chunked;
}

int HTTPClient::get_response_code() const {
    return response_num;
}

Error HTTPClient::get_response_headers(List<String> *r_response) {
    if (response_headers.empty())
        return ERR_INVALID_PARAMETER;

    for (const String &rhdr : response_headers) {
        r_response->emplace_back(rhdr);
    }

    response_headers.clear();

    return OK;
}

void HTTPClient::close() {
    if (tcp_connection->get_status() != StreamPeerTCP::STATUS_NONE)
        tcp_connection->disconnect_from_host();

    connection.unref();
    proxy_client.unref();
    status = STATUS_DISCONNECTED;
    head_request = false;
    if (resolving != IP::RESOLVER_INVALID_ID) {
        IP::get_singleton()->erase_resolve_item(resolving);
        resolving = IP::RESOLVER_INVALID_ID;
    }

    ip_candidates.clear();
    response_headers.clear();
    response_str.clear();
    body_size = -1;
    body_left = 0;
    chunk_left = 0;
    chunk_trailer_part = false;
    read_until_eof = false;
    response_num = 0;
    handshaking = false;
}

Error HTTPClient::poll() {
    switch (status) {
        case STATUS_RESOLVING: {
            ERR_FAIL_COND_V(resolving == IP::RESOLVER_INVALID_ID, ERR_BUG);

            IP::ResolverStatus rstatus = IP::get_singleton()->get_resolve_item_status(resolving);
            switch (rstatus) {
                case IP::RESOLVER_STATUS_WAITING:
                    return OK; // Still resolving

                case IP::RESOLVER_STATUS_DONE: {
                    ip_candidates = IP::get_singleton()->get_resolve_item_addresses(resolving);
                    IP::get_singleton()->erase_resolve_item(resolving);
                    resolving = IP::RESOLVER_INVALID_ID;
                    Error err = ERR_BUG; // Should be at least one entry.
                    while (ip_candidates.size() > 0) {
                        err = tcp_connection->connect_to_host(ip_candidates.pop_front().as<IP_Address>(), server_port);
                        if (err == OK) {
                            break;
                        }
                    }
                    if (err) {
                        status = STATUS_CANT_CONNECT;
                        return err;
                    }

                    status = STATUS_CONNECTING;
                } break;
                case IP::RESOLVER_STATUS_NONE:
                case IP::RESOLVER_STATUS_ERROR: {
                    IP::get_singleton()->erase_resolve_item(resolving);
                    resolving = IP::RESOLVER_INVALID_ID;
                    close();
                    status = STATUS_CANT_RESOLVE;
                    return ERR_CANT_RESOLVE;
                } break;
            }
        } break;
        case STATUS_CONNECTING: {
            StreamPeerTCP::Status s = tcp_connection->get_status();
            switch (s) {
                case StreamPeerTCP::STATUS_CONNECTING: {
                    return OK;
                }
                case StreamPeerTCP::STATUS_CONNECTED: {
                if (ssl && proxy_client) {
                    Error err = proxy_client->poll();
                    if (err == ERR_UNCONFIGURED) {
                        proxy_client->set_connection(tcp_connection);
                        const Vector<String> headers;
                        err = proxy_client->request(METHOD_CONNECT, FormatVE("%s:%d", conn_host.c_str(), conn_port), headers);
                        if (err != OK) {
                            status = STATUS_CANT_CONNECT;
                            return err;
                        }
                    } else if (err != OK) {
                        status = STATUS_CANT_CONNECT;
                        return err;
                    }
                    switch (proxy_client->get_status()) {
                        case STATUS_REQUESTING: {
                            return OK;
                        } break;
                        case STATUS_BODY: {
                            proxy_client->read_response_body_chunk();
                            return OK;
                        } break;
                        case STATUS_CONNECTED: {
                            if (proxy_client->get_response_code() != RESPONSE_OK) {
                                status = STATUS_CANT_CONNECT;
                                return ERR_CANT_CONNECT;
                            }
                            proxy_client.unref();
                            return OK;
                        }
                        case STATUS_DISCONNECTED:
                        case STATUS_RESOLVING:
                        case STATUS_CONNECTING: {
                            status = STATUS_CANT_CONNECT;
                            ERR_FAIL_V(ERR_BUG);
                        } break;
                        default: {
                            status = STATUS_CANT_CONNECT;
                            return ERR_CANT_CONNECT;
                        } break;
                    }
                } else if (ssl) {
                        Ref<StreamPeerSSL> ssl_impl;
                        if (!handshaking) {
                            // Connect the StreamPeerSSL and start handshaking
                            ssl_impl = Ref<StreamPeerSSL>(StreamPeerSSL::create(), DoNotAddRef);
                            ssl_impl->set_blocking_handshake_enabled(false);
                            Error err = ssl_impl->connect_to_stream(tcp_connection, ssl_verify_host, conn_host);
                            if (err != OK) {
                                close();
                                status = STATUS_SSL_HANDSHAKE_ERROR;
                                return ERR_CANT_CONNECT;
                            }
                            connection = ssl_impl;
                            handshaking = true;
                        } else {
                            // We are already handshaking, which means we can use your already active SSL connection
                            ssl_impl = dynamic_ref_cast<StreamPeerSSL>(connection);
                            if (not ssl_impl) {
                                close();
                                status = STATUS_SSL_HANDSHAKE_ERROR;
                                return ERR_CANT_CONNECT;
                            }

                            ssl_impl->poll(); // Try to finish the handshake
                        }

                        if (ssl_impl->get_status() == StreamPeerSSL::STATUS_CONNECTED) {
                            // Handshake has been successful
                            handshaking = false;
                            ip_candidates.clear();
                            status = STATUS_CONNECTED;
                            return OK;
                        } else if (ssl_impl->get_status() != StreamPeerSSL::STATUS_HANDSHAKING) {
                            // Handshake has failed
                            close();
                            status = STATUS_SSL_HANDSHAKE_ERROR;
                            return ERR_CANT_CONNECT;
                        }
                        // ... we will need to poll more for handshake to finish
                    } else {
                        ip_candidates.clear();
                        status = STATUS_CONNECTED;
                    }
                    return OK;
                }
                case StreamPeerTCP::STATUS_ERROR:
                case StreamPeerTCP::STATUS_NONE: {
                    Error err = ERR_CANT_CONNECT;
                    while (ip_candidates.size() > 0) {
                        tcp_connection->disconnect_from_host();
                        err = tcp_connection->connect_to_host(ip_candidates.pop_front().as<IP_Address>(), conn_port);
                        if (err == OK) {
                            return OK;
                        }
                    }
                    close();
                    status = STATUS_CANT_CONNECT;
                    return err;
                }
            }
        } break;
        case STATUS_BODY:
        case STATUS_CONNECTED: {
            // Check if we are still connected
            if (ssl) {
                Ref<StreamPeerSSL> tmp(dynamic_ref_cast<StreamPeerSSL>(connection));
                tmp->poll();
                if (tmp->get_status() != StreamPeerSSL::STATUS_CONNECTED) {
                    status = STATUS_CONNECTION_ERROR;
                    return ERR_CONNECTION_ERROR;
                }
            } else if (tcp_connection->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
                status = STATUS_CONNECTION_ERROR;
                return ERR_CONNECTION_ERROR;
            }
            // Connection established, requests can now be made
            return OK;
        }
        case STATUS_REQUESTING: {
            while (true) {
                uint8_t byte;
                int rec = 0;
                Error err = _get_http_data(&byte, 1, rec);
                if (err != OK) {
                    close();
                    status = STATUS_CONNECTION_ERROR;
                    return ERR_CONNECTION_ERROR;
                }

                if (rec == 0)
                    return OK; // Still requesting, keep trying!

                response_str.push_back(byte);
                int rs = response_str.size();
                if ((rs >= 2 && response_str[rs - 2] == '\n' && response_str[rs - 1] == '\n') ||
                        (rs >= 4 && response_str[rs - 4] == '\r' && response_str[rs - 3] == '\n' &&
                                response_str[rs - 2] == '\r' && response_str[rs - 1] == '\n')) {
                    // End of response, parse.
                    response_str.push_back(0);
                    String response((const char *)response_str.data(), response_str.size());
                    Vector<StringView> responses = StringUtils::split(response, '\n');
                    body_size = -1;
                    chunked = false;
                    body_left = 0;
                    chunk_left = 0;
                    chunk_trailer_part = false;
                    read_until_eof = false;
                    response_str.clear();
                    response_headers.clear();
                    response_num = RESPONSE_OK;

                    // Per the HTTP 1.1 spec, keep-alive is the default.
                    // Not following that specification breaks standard implementations.
                    // Broken web servers should be fixed.
                    bool keep_alive = true;

                    for (int i = 0; i < responses.size(); i++) {
                        StringView header = StringUtils::strip_edges(responses[i]);
                        String s = StringUtils::to_lower(header);
                        if (s.empty())
                            continue;
                        if (StringUtils::begins_with(s, "content-length:")) {
                            body_size = StringUtils::to_int64(
                                    StringUtils::strip_edges(StringUtils::substr(s, StringUtils::find(s, ":") + 1)));
                            body_left = body_size;

                        } else if (StringUtils::begins_with(s, "transfer-encoding:")) {
                            StringView encoding = StringUtils::strip_edges(
                                    StringUtils::substr(header, StringUtils::find(header, ":") + 1));
                            if (encoding == StringView("chunked")) {
                                chunked = true;
                            }
                        } else if (StringUtils::begins_with(s, "connection: close")) {
                            keep_alive = false;
                        }

                        if (i == 0 && StringUtils::begins_with(responses[i], "HTTP")) {
                            StringView num = StringUtils::get_slice(responses[i], ' ', 1);
                            response_num = StringUtils::to_int(num);
                        } else {
                            response_headers.push_back(String(header));
                        }
                    }
                    // This is a HEAD request, we won't receive anything.
                    if (head_request) {
                        body_size = 0;
                        body_left = 0;
                    }
                    if (body_size != -1 || chunked) {
                        status = STATUS_BODY;
                    } else if (!keep_alive) {
                        read_until_eof = true;
                        status = STATUS_BODY;
                    } else {
                        status = STATUS_CONNECTED;
                    }
                    return OK;
                }
            }
        } break;
        case STATUS_DISCONNECTED: {
            return ERR_UNCONFIGURED;
        } break;
        case STATUS_CONNECTION_ERROR:
        case STATUS_SSL_HANDSHAKE_ERROR: {
            return ERR_CONNECTION_ERROR;
        } break;
        case STATUS_CANT_CONNECT: {
            return ERR_CANT_CONNECT;
        } break;
        case STATUS_CANT_RESOLVE: {
            return ERR_CANT_RESOLVE;
        } break;
    }

    return OK;
}

int64_t HTTPClient::get_response_body_length() const {
    return body_size;
}

PoolByteArray HTTPClient::read_response_body_chunk() {
    ERR_FAIL_COND_V(status != STATUS_BODY, PoolByteArray());

    PoolByteArray ret;
    Error err = OK;

    if (chunked) {
        while (true) {
            if (chunk_trailer_part) {
                // We need to consume the trailer part too or keep-alive will break
                uint8_t b;
                int rec = 0;
                err = _get_http_data(&b, 1, rec);

                if (rec == 0)
                    break;

                chunk.push_back(b);
                int cs = chunk.size();
                if ((cs >= 2 && chunk[cs - 2] == '\r' && chunk[cs - 1] == '\n')) {
                    if (cs == 2) {
                        // Finally over
                        chunk_trailer_part = false;
                        status = STATUS_CONNECTED;
                        chunk.clear();
                        break;
                    } else {
                        // We do not process nor return the trailer data
                        chunk.clear();
                    }
                }
            } else if (chunk_left == 0) {
                // Reading length
                uint8_t b;
                int rec = 0;
                err = _get_http_data(&b, 1, rec);

                if (rec == 0)
                    break;

                chunk.push_back(b);

                if (chunk.size() > 32) {
                    ERR_PRINT("HTTP Invalid chunk hex len");
                    status = STATUS_CONNECTION_ERROR;
                    break;
                }

                if (chunk.size() > 2 && chunk[chunk.size() - 2] == '\r' && chunk[chunk.size() - 1] == '\n') {
                    int len = 0;
                    for (int i = 0; i < chunk.size() - 2; i++) {
                        char c = chunk[i];
                        int v = 0;
                        if (c >= '0' && c <= '9')
                            v = c - '0';
                        else if (c >= 'a' && c <= 'f')
                            v = c - 'a' + 10;
                        else if (c >= 'A' && c <= 'F')
                            v = c - 'A' + 10;
                        else {
                            ERR_PRINT("HTTP Chunk len not in hex!!");
                            status = STATUS_CONNECTION_ERROR;
                            break;
                        }
                        len <<= 4;
                        len |= v;
                        if (len > (1 << 24)) {
                            ERR_PRINT("HTTP Chunk too big!! >16mb");
                            status = STATUS_CONNECTION_ERROR;
                            break;
                        }
                    }

                    if (len == 0) {
                        // End reached!
                        chunk_trailer_part = true;
                        chunk.clear();
                        break;
                    }

                    chunk_left = len + 2;
                    chunk.resize(chunk_left);
                }
            } else {
                int rec = 0;
                err = _get_http_data(&chunk[chunk.size() - chunk_left], chunk_left, rec);
                if (rec == 0) {
                    break;
                }
                chunk_left -= rec;

                if (chunk_left == 0) {
                    if (chunk[chunk.size() - 2] != '\r' || chunk[chunk.size() - 1] != '\n') {
                        ERR_PRINT("HTTP Invalid chunk terminator (not \\r\\n)");
                        status = STATUS_CONNECTION_ERROR;
                        break;
                    }

                    ret.resize(chunk.size() - 2);
                    PoolByteArray::Write w = ret.write();
                    memcpy(w.ptr(), chunk.data(), chunk.size() - 2);
                    chunk.clear();
                }

                break;
            }
        }

    } else {
        int to_read = !read_until_eof ? MIN(body_left, read_chunk_size) : read_chunk_size;
        ret.resize(to_read);
        int _offset = 0;
        while (to_read > 0) {
            int rec = 0;
            {
                PoolByteArray::Write w = ret.write();
                err = _get_http_data(w.ptr() + _offset, to_read, rec);
            }
            if (rec <= 0) { // Ended up reading less
                ret.resize(_offset);
                break;
            } else {
                _offset += rec;
                to_read -= rec;
                if (!read_until_eof) {
                    body_left -= rec;
                }
            }
            if (err != OK) {
                ret.resize(_offset);
                break;
            }
        }
    }

    if (err != OK) {
        close();

        if (err == ERR_FILE_EOF) {
            status = STATUS_DISCONNECTED; // Server disconnected
        } else {
            status = STATUS_CONNECTION_ERROR;
        }
    } else if (body_left == 0 && !chunked && !read_until_eof) {
        status = STATUS_CONNECTED;
    }

    return ret;
}

HTTPClient::Status HTTPClient::get_status() const {
    return status;
}

void HTTPClient::set_blocking_mode(bool p_enable) {
    blocking = p_enable;
}

bool HTTPClient::is_blocking_mode_enabled() const {
    return blocking;
}

Error HTTPClient::_get_http_data(uint8_t *p_buffer, int p_bytes, int &r_received) {
    if (blocking) {
        // We can't use StreamPeer.get_data, since when reaching EOF we will get an
        // error without knowing how many bytes we received.
        Error err = ERR_FILE_EOF;
        int read = 0;
        int left = p_bytes;
        r_received = 0;
        while (left > 0) {
            err = connection->get_partial_data(p_buffer + r_received, left, read);
            if (err == OK) {
                r_received += read;
            } else if (err == ERR_FILE_EOF) {
                r_received += read;
                return err;
            } else {
                return err;
            }
            left -= read;
        }
        return err;
    } else {
        return connection->get_partial_data(p_buffer, p_bytes, r_received);
    }
}

void HTTPClient::set_read_chunk_size(int p_size) {
    ERR_FAIL_COND(p_size < 256 || p_size > (1 << 24));
    read_chunk_size = p_size;
}
int HTTPClient::get_read_chunk_size() const {
    return read_chunk_size;
}

HTTPClient::HTTPClient() {
    tcp_connection = make_ref_counted<StreamPeerTCP>();
    resolving = IP::RESOLVER_INVALID_ID;
    status = STATUS_DISCONNECTED;
    head_request = false;
    conn_port = -1;
    server_port = -1;
    http_proxy_port = -1;
    https_proxy_port = -1;
    body_size = -1;
    chunked = false;
    body_left = 0;
    read_until_eof = false;
    chunk_left = 0;
    chunk_trailer_part = false;
    response_num = 0;
    ssl = false;
    blocking = false;
    handshaking = false;
    // 64 KiB by default (favors fast download speeds at the cost of memory usage).
    read_chunk_size = 65536;
}

HTTPClient::~HTTPClient() = default;

#endif // #ifndef JAVASCRIPT_ENABLED

String HTTPClient::query_string_from_dict(const Dictionary &p_dict) {
    String query;
    auto keys = p_dict.get_key_list();
    for (int i = 0; i < keys.size(); ++i) {
        String encoded_key = StringUtils::http_escape(keys[i]);
        Variant value = p_dict[keys[i]];
        switch (value.get_type()) {
            case VariantType::ARRAY: {
                // Repeat the key with every values
                Array values = value.as<Array>();
                for (int j = 0; j < values.size(); ++j) {
                    query += "&" + encoded_key + "=" + StringUtils::http_escape(values[j].as<String>());
                }
                break;
            }
            case VariantType::NIL: {
                // Add the key with no value
                query += "&" + encoded_key;
                break;
            }
            default: {
                // Add the key-value pair
                query += "&" + encoded_key + "=" + StringUtils::http_escape(value.as<String>());
            }
        }
    }
    StringUtils::erase(query, 0, 1);
    return query;
}

void HTTPClient::set_http_proxy(const String &p_host, int p_port) {
    if (p_host.empty() || p_port == -1) {
        http_proxy_host.clear();
        http_proxy_port = -1;
    } else {
        http_proxy_host = p_host;
        http_proxy_port = p_port;
    }
}

void HTTPClient::set_https_proxy(const String &p_host, int p_port) {
    if (p_host.empty() || p_port == -1) {
        https_proxy_host.clear();
        https_proxy_port = -1;
    } else {
        https_proxy_host = p_host;
        https_proxy_port = p_port;
    }
}

Dictionary HTTPClient::get_response_headers_as_dictionary() {
    List<String> rh;
    get_response_headers(&rh);
    Dictionary ret;
    for (const String &s : rh) {
        auto sp = StringUtils::find(s, ":");
        if (sp == String::npos)
            continue;
        StringView key = StringUtils::strip_edges(StringUtils::substr(s, 0, sp));
        StringView value = StringUtils::strip_edges(StringUtils::substr(s, sp + 1, s.length()));
        ret[StringName(key)] = value;
    }

    return ret;
}

Vector<String> HTTPClient::_get_response_headers() {
    List<String> response_headers;
    get_response_headers(&response_headers);

    Vector<String> ret;
    ret.resize(response_headers.size());
    {
        int idx = 0;
        for (const String &s : response_headers) {
            ret[idx++] = s;
        }
    }
    return ret;
}

void HTTPClient::_bind_methods() {
    MethodBinder::bind_method(D_METHOD("connect_to_host", { "host", "port", "use_ssl", "verify_host" }),
            &HTTPClient::connect_to_host, { DEFVAL(-1), DEFVAL(false), DEFVAL(true) });
    SE_BIND_METHOD(HTTPClient,set_connection);
    SE_BIND_METHOD(HTTPClient,get_connection);
    MethodBinder::bind_method(
            D_METHOD("request_raw", { "method", "url", "headers", "body" }), &HTTPClient::request_raw);
    MethodBinder::bind_method(
            D_METHOD("request", { "method", "url", "headers", "body" }), &HTTPClient::request, { DEFVAL(String()) });
    SE_BIND_METHOD(HTTPClient,close);

    SE_BIND_METHOD(HTTPClient,has_response);
    SE_BIND_METHOD(HTTPClient,is_response_chunked);
    SE_BIND_METHOD(HTTPClient,get_response_code);
    MethodBinder::bind_method(D_METHOD("get_response_headers"), &HTTPClient::_get_response_headers);
    MethodBinder::bind_method(
            D_METHOD("get_response_headers_as_dictionary"), &HTTPClient::get_response_headers_as_dictionary);
    SE_BIND_METHOD(HTTPClient,get_response_body_length);
    SE_BIND_METHOD(HTTPClient,read_response_body_chunk);
    SE_BIND_METHOD(HTTPClient,set_read_chunk_size);
    SE_BIND_METHOD(HTTPClient,get_read_chunk_size);

    SE_BIND_METHOD(HTTPClient,set_blocking_mode);
    SE_BIND_METHOD(HTTPClient,is_blocking_mode_enabled);

    SE_BIND_METHOD(HTTPClient,get_status);
    SE_BIND_METHOD(HTTPClient,poll);

    SE_BIND_METHOD(HTTPClient,set_http_proxy);
    SE_BIND_METHOD(HTTPClient,set_https_proxy);


    SE_BIND_METHOD(HTTPClient,query_string_from_dict);

    ADD_PROPERTY(
            PropertyInfo(VariantType::BOOL, "blocking_mode_enabled"), "set_blocking_mode", "is_blocking_mode_enabled");
    ADD_PROPERTY(PropertyInfo(VariantType::OBJECT, "connection", PropertyHint::ResourceType, "StreamPeer", 0),
            "set_connection", "get_connection");
    ADD_PROPERTY(PropertyInfo(VariantType::INT, "read_chunk_size", PropertyHint::Range, "256,16777216"),
            "set_read_chunk_size", "get_read_chunk_size");

    BIND_ENUM_CONSTANT(METHOD_GET);
    BIND_ENUM_CONSTANT(METHOD_HEAD);
    BIND_ENUM_CONSTANT(METHOD_POST);
    BIND_ENUM_CONSTANT(METHOD_PUT);
    BIND_ENUM_CONSTANT(METHOD_DELETE);
    BIND_ENUM_CONSTANT(METHOD_OPTIONS);
    BIND_ENUM_CONSTANT(METHOD_TRACE);
    BIND_ENUM_CONSTANT(METHOD_CONNECT);
    BIND_ENUM_CONSTANT(METHOD_PATCH);
    BIND_ENUM_CONSTANT(METHOD_MAX);

    BIND_ENUM_CONSTANT(STATUS_DISCONNECTED);
    BIND_ENUM_CONSTANT(STATUS_RESOLVING); // Resolving hostname (if hostname was passed in)
    BIND_ENUM_CONSTANT(STATUS_CANT_RESOLVE);
    BIND_ENUM_CONSTANT(STATUS_CONNECTING); // Connecting to IP
    BIND_ENUM_CONSTANT(STATUS_CANT_CONNECT);
    BIND_ENUM_CONSTANT(STATUS_CONNECTED); // Connected, now accepting requests
    BIND_ENUM_CONSTANT(STATUS_REQUESTING); // Request in progress
    BIND_ENUM_CONSTANT(STATUS_BODY); // Request resulted in body which must be read
    BIND_ENUM_CONSTANT(STATUS_CONNECTION_ERROR);
    BIND_ENUM_CONSTANT(STATUS_SSL_HANDSHAKE_ERROR);

    BIND_ENUM_CONSTANT(RESPONSE_CONTINUE);
    BIND_ENUM_CONSTANT(RESPONSE_SWITCHING_PROTOCOLS);
    BIND_ENUM_CONSTANT(RESPONSE_PROCESSING);

    // 2xx successful
    BIND_ENUM_CONSTANT(RESPONSE_OK);
    BIND_ENUM_CONSTANT(RESPONSE_CREATED);
    BIND_ENUM_CONSTANT(RESPONSE_ACCEPTED);
    BIND_ENUM_CONSTANT(RESPONSE_NON_AUTHORITATIVE_INFORMATION);
    BIND_ENUM_CONSTANT(RESPONSE_NO_CONTENT);
    BIND_ENUM_CONSTANT(RESPONSE_RESET_CONTENT);
    BIND_ENUM_CONSTANT(RESPONSE_PARTIAL_CONTENT);
    BIND_ENUM_CONSTANT(RESPONSE_MULTI_STATUS);
    BIND_ENUM_CONSTANT(RESPONSE_ALREADY_REPORTED);
    BIND_ENUM_CONSTANT(RESPONSE_IM_USED);

    // 3xx redirection
    BIND_ENUM_CONSTANT(RESPONSE_MULTIPLE_CHOICES);
    BIND_ENUM_CONSTANT(RESPONSE_MOVED_PERMANENTLY);
    BIND_ENUM_CONSTANT(RESPONSE_FOUND);
    BIND_ENUM_CONSTANT(RESPONSE_SEE_OTHER);
    BIND_ENUM_CONSTANT(RESPONSE_NOT_MODIFIED);
    BIND_ENUM_CONSTANT(RESPONSE_USE_PROXY);
    BIND_ENUM_CONSTANT(RESPONSE_SWITCH_PROXY);
    BIND_ENUM_CONSTANT(RESPONSE_TEMPORARY_REDIRECT);
    BIND_ENUM_CONSTANT(RESPONSE_PERMANENT_REDIRECT);

    // 4xx client error
    BIND_ENUM_CONSTANT(RESPONSE_BAD_REQUEST);
    BIND_ENUM_CONSTANT(RESPONSE_UNAUTHORIZED);
    BIND_ENUM_CONSTANT(RESPONSE_PAYMENT_REQUIRED);
    BIND_ENUM_CONSTANT(RESPONSE_FORBIDDEN);
    BIND_ENUM_CONSTANT(RESPONSE_NOT_FOUND);
    BIND_ENUM_CONSTANT(RESPONSE_METHOD_NOT_ALLOWED);
    BIND_ENUM_CONSTANT(RESPONSE_NOT_ACCEPTABLE);
    BIND_ENUM_CONSTANT(RESPONSE_PROXY_AUTHENTICATION_REQUIRED);
    BIND_ENUM_CONSTANT(RESPONSE_REQUEST_TIMEOUT);
    BIND_ENUM_CONSTANT(RESPONSE_CONFLICT);
    BIND_ENUM_CONSTANT(RESPONSE_GONE);
    BIND_ENUM_CONSTANT(RESPONSE_LENGTH_REQUIRED);
    BIND_ENUM_CONSTANT(RESPONSE_PRECONDITION_FAILED);
    BIND_ENUM_CONSTANT(RESPONSE_REQUEST_ENTITY_TOO_LARGE);
    BIND_ENUM_CONSTANT(RESPONSE_REQUEST_URI_TOO_LONG);
    BIND_ENUM_CONSTANT(RESPONSE_UNSUPPORTED_MEDIA_TYPE);
    BIND_ENUM_CONSTANT(RESPONSE_REQUESTED_RANGE_NOT_SATISFIABLE);
    BIND_ENUM_CONSTANT(RESPONSE_EXPECTATION_FAILED);
    BIND_ENUM_CONSTANT(RESPONSE_IM_A_TEAPOT);
    BIND_ENUM_CONSTANT(RESPONSE_MISDIRECTED_REQUEST);
    BIND_ENUM_CONSTANT(RESPONSE_UNPROCESSABLE_ENTITY);
    BIND_ENUM_CONSTANT(RESPONSE_LOCKED);
    BIND_ENUM_CONSTANT(RESPONSE_FAILED_DEPENDENCY);
    BIND_ENUM_CONSTANT(RESPONSE_UPGRADE_REQUIRED);
    BIND_ENUM_CONSTANT(RESPONSE_PRECONDITION_REQUIRED);
    BIND_ENUM_CONSTANT(RESPONSE_TOO_MANY_REQUESTS);
    BIND_ENUM_CONSTANT(RESPONSE_REQUEST_HEADER_FIELDS_TOO_LARGE);
    BIND_ENUM_CONSTANT(RESPONSE_UNAVAILABLE_FOR_LEGAL_REASONS);

    // 5xx server error
    BIND_ENUM_CONSTANT(RESPONSE_INTERNAL_SERVER_ERROR);
    BIND_ENUM_CONSTANT(RESPONSE_NOT_IMPLEMENTED);
    BIND_ENUM_CONSTANT(RESPONSE_BAD_GATEWAY);
    BIND_ENUM_CONSTANT(RESPONSE_SERVICE_UNAVAILABLE);
    BIND_ENUM_CONSTANT(RESPONSE_GATEWAY_TIMEOUT);
    BIND_ENUM_CONSTANT(RESPONSE_HTTP_VERSION_NOT_SUPPORTED);
    BIND_ENUM_CONSTANT(RESPONSE_VARIANT_ALSO_NEGOTIATES);
    BIND_ENUM_CONSTANT(RESPONSE_INSUFFICIENT_STORAGE);
    BIND_ENUM_CONSTANT(RESPONSE_LOOP_DETECTED);
    BIND_ENUM_CONSTANT(RESPONSE_NOT_EXTENDED);
    BIND_ENUM_CONSTANT(RESPONSE_NETWORK_AUTH_REQUIRED);
}
