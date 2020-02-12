/*
 * Copyright (C) 2016 Jon Sten
 * 
 * This program is free software: you can redistribute it and/or modify    
 * it under the terms of the GNU General Public License as published by    
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,    
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License    
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ZabbixAgent_h
#define ZabbixAgent_h
#include <WiFiServer.h>
#include <WiFiClient.h>

class ZabbixResponseHandler;

class ZabbixAgent : public WiFiServer {
  public:
    typedef void (*CallbackType)(ZabbixResponseHandler, const char*, const int, const char**);
    ZabbixAgent();
    ZabbixAgent(uint16_t port);
    ~ZabbixAgent();
    void on(char* key, CallbackType fn);
    void handleClient();
    friend class ZabbixResponseHandler;
  private:
    WiFiClient currentClient = WiFiClient();
    
    enum ParseState {PARSE_KEY, PARAMETER_START, UNQUOTED_STRING, QUOTED_STRING, ARRAY, END, ERROR};

    ParseState parseState;
    String currentParseToken;
    char* currentKey = 0;
    char** currentParameters;
    uint8_t currentParametersLen;
    uint8_t currentNumParameters;
    
    class Handler {
      public:
        Handler(char* key, CallbackType fn): key(key), fn(fn) {
        }
        char* key;
        CallbackType fn;
        Handler* next = 0;
    };
    Handler* firstHandler = 0;
    Handler* lastHandler = 0;

    void processCommand();
    void copyParameterToCurrent();
    void respond(const char* resp, const uint16_t len);
    void error(const char* msg);
};

class ZabbixResponseHandler {
  public:
    ZabbixResponseHandler(ZabbixAgent* server): server(server) {
    }
    void respond(const unsigned int resp);
    void respond(const int resp);
    void respond(const char* resp);
    void respond(const double resp);
    void respond(const double resp, const unsigned char prec);
    void respond(const char* resp, const uint16_t len);
    void error(const char* msg);
  private:
    ZabbixAgent* server;
};

#endif
