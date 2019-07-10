#ifndef __COUCHDB_XX__
#define __COUCHDB_XX__

#include <string>
#include <curl/curl.h>
#include "jsonxx.h"
#include "curlrequest.hpp"

namespace wezside
{
    class CouchDBXX
    {
    private:
        std::string uri;
        CurlRequest curlrequest;

        jsonxx::Object request(std::string req, std::string req_method = "GET", std::string req_data = "")
        {
            jsonxx::Object o;
            std::ostringstream res;
            if (CURLE_OK == curlrequest.curl_read(req, res, 10, req_method, req_data))
            {
                o.parse(res.str().c_str());
            }			
            else
            {
                o << "error" << "An error occured when CURL request was made.";
                o << "code" << 100;  
            } 
            return o;
        }

    public:
        CouchDBXX() : uri("http://127.0.0.1:5984/"){}
        ~CouchDBXX() {}

        void set_db(std::string uri)
        {
            this->uri = uri;
        }
        std::string get_db()
        {
            return this->uri;
        }
        jsonxx::Object put(std::string db_name)
        {
            if (db_name.empty()) return jsonxx::Object("error", "Database name cannot be empty.");
            if (uri.at(uri.length()-1) != '/') uri += '/';
            return request(uri + db_name, "PUT");
        }
        jsonxx::Object put(std::string db_name, jsonxx::Object body)
        {
            if (db_name.empty()) return jsonxx::Object("error", "Database name cannot be empty.");
            if (uri.at(uri.length()-1) != '/') uri += '/';
            return request(uri + db_name, "POST", body.json());
        }		
        jsonxx::Object put(std::string db_name, std::string design_name, std::string filename)
        {
            if (db_name.empty()) return jsonxx::Object("error", "Database name cannot be empty.");
            if (uri.at(uri.length()-1) != '/') uri += '/';
            
            std::string out;
            std::string line;
            std::ifstream myfile(filename.c_str());
            if (myfile.is_open())
            {
                while (getline(myfile, line)) out += line;
                myfile.close();
            }
            else return jsonxx::Object("error", "Unable to open file");
            return request(uri + db_name + "/_design/" + design_name, "PUT", out);
        }
        jsonxx::Object del(std::string db_name)
        {
            if (db_name.empty()) return jsonxx::Object("error", "Database name cannot be empty.");
            if (uri.at(uri.length()-1) != '/') uri += '/';
            return request(uri + db_name, "DELETE");	
        }
        jsonxx::Object view(std::string db_name, std::string design, std::string view, std::string params = "")
        {
            if (db_name.empty()) return jsonxx::Object("error", "Database name cannot be empty.");
            if (uri.at(uri.length()-1) != '/') uri += '/';
            std::string p = params.empty() ? "" : + "?" + params;
            return request(uri + db_name + "/_design/" + design + "/_view/" + view  + p);
        }
        jsonxx::Object view(std::string db_name, std::string design, std::string view, jsonxx::Object params)
        {	
            if (db_name.empty()) return jsonxx::Object("error", "Database name cannot be empty.");
            if (uri.at(uri.length()-1) != '/') uri += '/';
            return request(uri + db_name + "/_design/" + design + "/_view/" + view, "POST", params.json());
        }
        jsonxx::Object doc(std::string db_name, std::string doc_name)
        {
            if (db_name.empty()) return jsonxx::Object("error", "Database name cannot be empty.");
            if (uri.at(uri.length()-1) != '/') uri += '/';
            return request(uri + db_name + "/" + doc_name);
        }
        jsonxx::Object get(std::string db_name, std::string design_name)
        {
            if (db_name.empty()) return jsonxx::Object("error", "Database name cannot be empty.");
            if (uri.at(uri.length()-1) != '/') uri += '/';
            return request(uri + db_name + "/_design/" + design_name);
        }
    };
}
#endif // __COUCHDB_XX__
