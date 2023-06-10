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

#include <ESP8266ZabbixUtil.h>
#include <ZabbixAgent.h>
#include <ESP8266WiFi.h>

static char* hostname = 0;

void respondAgentPing(ZabbixResponseHandler handler, const char* address, const int nparam, const char** params) {
  handler.respond(1);
}

void respondAgentVersion(ZabbixResponseHandler handler, const char* address, const int nparam, const char** params) {
  handler.respond("AZSAL_1.0");
}

void respondWIFIRSSI(ZabbixResponseHandler handler, const char* address, const int nparam, const char** params) {
  handler.respond(WiFi.RSSI());
}

void respondWIFISSID(ZabbixResponseHandler handler, const char* address, const int nparam, const char** params) {
  handler.respond(WiFi.SSID().c_str());
}

void respondWIFIChannel(ZabbixResponseHandler handler, const char* address, const int nparam, const char** params) {
  handler.respond(WiFi.channel());
}

void respondChipID(ZabbixResponseHandler handler, const char* address, const int nparam, const char** params) {
  char resp[9];
  snprintf(resp, 9, "%08X", ESP.getChipId());
  handler.respond(resp);
}

void respondAgentHostname(ZabbixResponseHandler handler, const char* address, const int nparam, const char** params) {
	if (hostname) {
		handler.respond(hostname);
	} else {
		respondChipID(handler, address, nparam, params);
	}
}

void respondChipFreeHeap(ZabbixResponseHandler handler, const char* address, const int nparam, const char** params) {
  handler.respond(ESP.getFreeHeap());
}

void registerESP8266ZabbixUtil(ZabbixAgent &agent) {
	registerESP8266ZabbixUtil(agent, 0);
}
void registerESP8266ZabbixUtil(ZabbixAgent &agent, char* _hostname) {
	if (_hostname) {
		hostname = (char*) malloc(strlen(_hostname) + 1);
		strcpy(hostname, _hostname);
	}
  agent.on("agent.ping", respondAgentPing);
  agent.on("agent.hostname", respondAgentHostname);
  agent.on("agent.version", respondAgentVersion);
  agent.on("wifi.rssi", respondWIFIRSSI);
  agent.on("wifi.ssid", respondWIFISSID);
  agent.on("wifi.channel", respondWIFIChannel);
  agent.on("chip.id", respondChipID);
  agent.on("chip.freeHeap", respondChipFreeHeap);
}

