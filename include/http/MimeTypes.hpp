#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include <string>
#include <map>

class MimeTypes {
public:
    static std::string getContentType(const std::string &path) {
        size_t dot = path.find_last_of(".");
        if (dot == std::string::npos)
            return "application/octet-stream";

        std::string ext = path.substr(dot + 1);
        
        // Use static map for O(1) lookup instead of repeated if-else chain
        static std::map<std::string, std::string> mimeMap;
        static bool initialized = false;
        
        if (!initialized) {
            mimeMap["html"] = "text/html";
            mimeMap["htm"] = "text/html";
            mimeMap["css"] = "text/css";
            mimeMap["js"] = "application/javascript";
            mimeMap["json"] = "application/json";
            mimeMap["png"] = "image/png";
            mimeMap["jpg"] = "image/jpeg";
            mimeMap["jpeg"] = "image/jpeg";
            mimeMap["gif"] = "image/gif";
            mimeMap["svg"] = "image/svg+xml";
            mimeMap["pdf"] = "application/pdf";
            mimeMap["txt"] = "text/plain";
            initialized = true;
        }
        
        std::map<std::string, std::string>::const_iterator it = mimeMap.find(ext);
        if (it != mimeMap.end())
            return it->second;
        return "application/octet-stream";
    }
};

#endif
