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

#include "ZabbixAgent.h"

ZabbixAgent::ZabbixAgent() : ZabbixAgent(10050) {
}

ZabbixAgent::ZabbixAgent(uint16_t port) : WiFiServer(port) {
  currentParametersLen = 4;
  currentParameters = new char*[currentParametersLen]();
}

ZabbixAgent::~ZabbixAgent() {
  if (currentKey) {
    delete[] currentKey;
  }
  for (uint8_t i = 0; i < currentParametersLen; i++) {
    if (currentParameters[i]) {
      delete[] currentParameters[i];
    }
  }
  delete[] currentParameters;
}

void ZabbixAgent::handleClient() {
  if (!currentClient || !currentClient.connected()) {
    if (!currentClient.connected()) {
      currentClient.stop();
    }
    currentClient = available();
    parseState = PARSE_KEY;
    currentParseToken = "";
    currentNumParameters = 0;
  }
  if (currentClient && currentClient.connected()) {
    // Parser based on: https://www.zabbix.com/documentation/3.2/manual/config/items/item/key
    int b;
    while (parseState != END && parseState != ERROR && (b = currentClient.read()) >= 0) {
      switch (parseState) {
      case PARSE_KEY:
        if (('0' <= b && b <= '9') || ('a' <= b && b <= 'z') || ('A' <= b && b <= 'Z') || b == '_' || b == '-' || b == '.') {
          currentParseToken += (char) b;
        } else if (b == '[') {
          parseState = PARAMETER_START;
        } else if (b == '\r' || b == '\n') {
          parseState = END;
        } else {
          error("Unexpected token.");
          parseState = ERROR;
          break;
        }
        if (parseState != PARSE_KEY) {
          if (currentKey) {
            delete[] currentKey;
          }
          currentKey = new char[currentParseToken.length() + 1];
          currentParseToken.getBytes((byte*)currentKey, currentParseToken.length() + 1);
          currentKey[currentParseToken.length()] = 0;
          currentParseToken = "";
        }
        break;
      case PARAMETER_START:
        currentNumParameters++;
        if (currentNumParameters > currentParametersLen) {
          currentParametersLen *= 2;
          char** old = currentParameters;
          currentParameters = new char*[currentParametersLen]();
          memcpy(currentParameters, old, sizeof(char*));
          delete[] old;
        }
        if (b == '"') {
          parseState = QUOTED_STRING;
          break;
        } else if (b == '[') {
          parseState = ARRAY;
          break;
        }
        parseState = UNQUOTED_STRING;
      case UNQUOTED_STRING:
        if (b == ',') {
          parseState = PARAMETER_START;
        } else if (b == ']') {
          parseState = END;
        } else {
          currentParseToken += (char) b;
        }
        if (parseState != UNQUOTED_STRING) {
          if (currentParameters[currentNumParameters - 1]) {
            delete[] currentParameters[currentNumParameters - 1];
          }
          currentParameters[currentNumParameters - 1] = new char[currentParseToken.length() + 1];
          currentParseToken.getBytes((byte*)currentParameters[currentNumParameters - 1], currentParseToken.length() + 1);
          currentParameters[currentNumParameters - 1][currentParseToken.length()] = 0;
          currentParseToken = "";
        }
        break;
      case QUOTED_STRING:
        error("QSTRING not supported.");
        parseState = ERROR;
        break;
      case ARRAY:
        error("Array not supported.");
        parseState = ERROR;
        break;
      }
    }
    if (parseState == END || parseState == ERROR) {
      if (parseState == END) {
        processCommand();
      }
      currentClient.stop();
      if (currentKey) {
        delete[] currentKey;
        currentKey = 0;
      }
    }
  }
}

void ZabbixResponseHandler::respond(const int resp) {
    char buff[30];
    snprintf(buff, 30, "%d", resp);
    respond(buff);
}

void ZabbixResponseHandler::respond(const char* resp) {
  respond(resp, strlen(resp));
}

void ZabbixResponseHandler::respond(const char* resp, const uint16_t len) {
  server->respond(resp, len);
}

void ZabbixResponseHandler::error(const char* msg) {
  server->error(msg);
}


void ZabbixAgent::respond(const char* resp, const uint16_t len) {
  if (!currentClient) {
    return;
  }
  /**
   * From: https://www.zabbix.com/documentation/3.2/manual/appendix/items/activepassive?s[]=agent&s[]=protocol
   * <HEADER> - "ZBXD\x01" (5 bytes)
   * <DATALEN> - data length (8 bytes). 1 will be formatted as 01/00/00/00/00/00/00/00 (eight bytes in HEX, 64 bit number)
   */
   const char header[] = {'Z','B','X','D',0x01,len & 0xFF, (len >> 8) & 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   currentClient.write(header, 13);
   currentClient.write(resp, len);
}

void ZabbixAgent::error(const char* msg) {
	// len = 17 for ZBX_NOTSUPPORTED\x00 + len(msg) + 1 for null
	uint16_t len = 17 + strlen(msg) + 1;
	char resp[len];
	
	strcpy(resp, "ZBX_NOTSUPPORTED");
	strcpy(&resp[17], msg);
	respond(resp, len);
}

void ZabbixAgent::processCommand() {
  ZabbixAgent::Handler* handler = firstHandler;
  while (handler) {
    if (strcmp(currentKey, handler->key) == 0) {
      handler->fn(ZabbixResponseHandler(this), currentKey, currentNumParameters, (const char**) currentParameters);
      return;
    }
    handler = handler->next;
  }
  error("Unknown command.");
}

void ZabbixAgent::on(char* key, ZabbixAgent::CallbackType fn) {
  ZabbixAgent::Handler* handler = new ZabbixAgent::Handler(key, fn);
  if (lastHandler) {
    lastHandler->next = handler;
    lastHandler = handler;
  } else {
    firstHandler = handler;
    lastHandler = handler;
  }
}

