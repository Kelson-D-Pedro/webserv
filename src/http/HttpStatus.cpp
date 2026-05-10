#include "../../include/http/HttpStatus.hpp"

const int HttpStatus::OK = 200;
const int HttpStatus::CREATED = 201;
const int HttpStatus::NO_CONTENT = 204;

const int HttpStatus::MOVED_PERMANENTLY = 301;
const int HttpStatus::FOUND = 302;

const int HttpStatus::BAD_REQUEST = 400;
const int HttpStatus::FORBIDDEN = 403;
const int HttpStatus::NOT_FOUND = 404;
const int HttpStatus::METHOD_NOT_ALLOWED = 405;
const int HttpStatus::REQUEST_TIMEOUT = 408;
const int HttpStatus::PAYLOAD_TOO_LARGE = 413;

const int HttpStatus::INTERNAL_SERVER_ERROR = 500;
const int HttpStatus::NOT_IMPLEMENTED = 501;
const int HttpStatus::BAD_GATEWAY = 502;
const int HttpStatus::SERVICE_UNAVAILABLE = 503;
const int HttpStatus::GATEWAY_TIMEOUT = 504;
const int HttpStatus::HTTP_VERSION_NOT_SUPPORTED = 505;

std::map<int, std::string> HttpStatus::initMap()
{
    std::map<int, std::string> m;
    m[200] = "OK";
    m[201] = "Created";
    m[204] = "No Content";
    m[301] = "Moved Permanently";
    m[302] = "Found";
    m[400] = "Bad Request";
    m[403] = "Forbidden";
    m[404] = "Not Found";
    m[405] = "Method Not Allowed";
    m[408] = "Request Timeout";
    m[413] = "Payload Too Large";
    m[500] = "Internal Server Error";
    m[501] = "Not Implemented";
    m[502] = "Bad Gateway";
    m[503] = "Service Unavailable";
    m[504] = "Gateway Timeout";
    m[505] = "HTTP Version Not Supported";
    return m;
}

const std::map<int, std::string> HttpStatus::_statusMap = HttpStatus::initMap();

std::string HttpStatus::getMessage(int code)
{
    std::map<int, std::string>::const_iterator it = _statusMap.find(code);
    if (it != _statusMap.end())
        return it->second;
    return "Unknown Status";
}

bool HttpStatus::isInformational(int code) { return code >= 100 && code < 200; }
bool HttpStatus::isSuccess(int code) { return code >= 200 && code < 300; }
bool HttpStatus::isRedirect(int code) { return code >= 300 && code < 400; }
bool HttpStatus::isClientError(int code) { return code >= 400 && code < 500; }
bool HttpStatus::isServerError(int code) { return code >= 500 && code < 600; }
