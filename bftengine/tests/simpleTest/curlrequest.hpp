#ifndef __FILE_DOWNLOADER__
#define __FILE_DOWNLOADER__

#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace wezside
{
    class CurlRequest
    {

    public:

        CurlRequest()
        {
            curl_global_init(CURL_GLOBAL_ALL);
        };

        CURLcode status;

        // callback function writes data to a std::ostream
        static size_t data_write(void* buf, size_t size, size_t nmemb, void* userp)
        {
            if(userp)
            {
                std::ostream& os = *static_cast<std::ostream*>(userp);
                std::streamsize len = size * nmemb;
                if(os.write(static_cast<char*>(buf), len))
                    return len;
            }
            return 0;
        }

        /**
         * timeout is in seconds
         **/
        CURLcode curl_read(const std::string& url, 
                           std::ostream& os, 
                           long timeout = 30, 
                           std::string custom_req = "GET", 
                           std::string data = "",
                           std::string ssl_cert_path = "",
                           std::string ssl_cert_key_path = "")
        {
            CURLcode code(CURLE_FAILED_INIT);
            CURL* curl = curl_easy_init();
            if(curl)
            {
                curl_slist *headers = NULL;
                headers = curl_slist_append(headers, "Accept: application/json"); 
                headers = curl_slist_append(headers, "Content-Type: application/json"); 
                headers = curl_slist_append(headers, "charsets: utf-8");
                if(CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FILE, &os))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str()))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str()))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_VERBOSE, false))

                // SSL Options
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSLCERT, ssl_cert_path.c_str()))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSLKEY, ssl_cert_key_path.c_str()))

                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, custom_req.c_str())))
                {
                    code = curl_easy_perform(curl);
                }
                curl_easy_cleanup(curl);
            }
            status = code;
            return code;
        }
    };
}
#endif // __FILE_DOWNLOADER__
#
