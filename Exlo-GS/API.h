#pragma once
#include "pch.h"

#include <wininet.h>
#include <variant>
#include <thread>
#pragma comment(lib, "wininet.lib")

using JsonVal = std::variant<int, std::string>;

namespace API {
    static inline std::string Json(std::initializer_list<std::pair<std::string, JsonVal>> v) 
    {
        std::string r = "{";

        bool first = true;

        for (auto& p : v) {
            if (!first)
                r += ",";

            first = false;
            r += "\"" + p.first + "\":";

            if (std::holds_alternative<int>(p.second))
                r += std::to_string(std::get<int>(p.second));
            else
                r += "\"" + std::get<std::string>(p.second) + "\"";
        }

        r += "}";
        return r;
    }

    static inline std::string ReadResponse(HINTERNET request)
    {
        std::string response;
        char buffer[4096];
        DWORD bytesRead = 0;

        while (InternetReadFile(request, buffer, sizeof(buffer), &bytesRead) && bytesRead)
            response.append(buffer, bytesRead);

        return response;
    }

    static inline std::string Get(const std::string& url) {
        HINTERNET hInternet = InternetOpenA("FAH", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        std::string response;

        if (hInternet) {
            HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);

            if (hConnect) {
                response = ReadResponse(hConnect);
                InternetCloseHandle(hConnect);
            }

            InternetCloseHandle(hInternet);
        }

        return response;
    }

    static inline std::string Post(const std::string& url, const std::string& data) {
        URL_COMPONENTSA urlComp{};
        char host[256]{};
        char path[1024]{};

        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = host;
        urlComp.dwHostNameLength = sizeof(host);
        urlComp.lpszUrlPath = path;
        urlComp.dwUrlPathLength = sizeof(path);

        InternetCrackUrlA(url.c_str(), 0, 0, &urlComp);

        HINTERNET hInternet = InternetOpenA("API Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        std::string response;

        if (hInternet) {
            HINTERNET hConnect = InternetConnectA(hInternet, host, urlComp.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

            if (hConnect) {
                HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);

                if (hRequest) {
                    const char* headers = "Content-Type: application/json\r\n";
                    HttpSendRequestA(hRequest, headers, strlen(headers), (LPVOID)data.c_str(), data.size());
                    response = ReadResponse(hRequest);
                    InternetCloseHandle(hRequest);
                }

                InternetCloseHandle(hConnect);
            }

            InternetCloseHandle(hInternet);
        }

        return response;
    }

    static inline void GetAsync(const std::string& url) {
        std::thread([url]() {
            HINTERNET hInternet = InternetOpenA("API Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

            if (hInternet) {
                HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);

                if (hConnect) {
                    ReadResponse(hConnect);
                    InternetCloseHandle(hConnect);
                }

                InternetCloseHandle(hInternet);
            }
            }).detach();
    }

    static inline void PostAsync(const std::string& url, const std::string& data) {
        std::thread([url, data]() {
            URL_COMPONENTSA urlComp{};
            char host[256]{};
            char path[1024]{};

            urlComp.dwStructSize = sizeof(urlComp);
            urlComp.lpszHostName = host;
            urlComp.dwHostNameLength = sizeof(host);
            urlComp.lpszUrlPath = path;
            urlComp.dwUrlPathLength = sizeof(path);

            InternetCrackUrlA(url.c_str(), 0, 0, &urlComp);

            HINTERNET hInternet = InternetOpenA("API Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

            if (hInternet) {
                HINTERNET hConnect = InternetConnectA(hInternet, host, urlComp.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

                if (hConnect) {
                    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);

                    if (hRequest) {
                        const char* headers = "Content-Type: application/json\r\n";
                        HttpSendRequestA(hRequest, headers, strlen(headers), (LPVOID)data.c_str(), data.size());
                        ReadResponse(hRequest);
                        InternetCloseHandle(hRequest);
                    }

                    InternetCloseHandle(hConnect);
                }

                InternetCloseHandle(hInternet);
            }
            }).detach();
    }
}