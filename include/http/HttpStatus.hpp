#ifndef HTTPSTATUS_HPP
#define HTTPSTATUS_HPP

#include <string>
#include <map>

class HttpStatus {
public:
   
    static const int OK;                     
    static const int CREATED;                
    static const int NO_CONTENT;            

    static const int MOVED_PERMANENTLY;      
    static const int FOUND;                  

    static const int BAD_REQUEST;            
    static const int FORBIDDEN;              
    static const int NOT_FOUND;              
    static const int METHOD_NOT_ALLOWED;    
    static const int REQUEST_TIMEOUT;      
    static const int PAYLOAD_TOO_LARGE;     

    static const int INTERNAL_SERVER_ERROR; 
    static const int NOT_IMPLEMENTED;        
    static const int BAD_GATEWAY;           
    static const int SERVICE_UNAVAILABLE;   
    static const int GATEWAY_TIMEOUT;   
    static const int HTTP_VERSION_NOT_SUPPORTED;   

    static std::string getMessage(int code);
    static bool isInformational(int code); 
    static bool isSuccess(int code);        
    static bool isRedirect(int code);      
    static bool isClientError(int code);   
    static bool isServerError(int code);  

private:
    static std::map<int, std::string> initMap();
    static const std::map<int, std::string> _statusMap;
};

#endif