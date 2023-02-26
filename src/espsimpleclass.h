#include "Arduino.h"
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "Preferences.h"
#include "SPIFFS.h"

#pragma once
#ifndef __ESPSIMPLE_CLASS_H__
#define __ESPSIMPLE_CLASS_H__

class ESPSimpleClass { 
    public:
        ESPSimpleClass();
        bool begin(String device_id, String friendly_name, String model, String sw_version = __VERSION__);
        bool begin(String friendly_name, String model, String sw_version = __VERSION__);
        bool send_state(char *uid, char *state);
        bool register_sensor(char *uid, char *name, char *unit, char *state_class, char *device_class);  
        bool reset_adoption();
        bool is_adopted()
        {
            return me_is_adopted;
        }
    private:   
        AsyncWebServer server = AsyncWebServer(8901);
        String ha_hostname;
        String my_device_id;
        String my_friendly_name;
        String my_model;
        String encryption_key;
        String my_sw_version;
        IPAddress ha_ip;
        bool me_is_adopted;
        bool ha_found;
        int ha_port = 8901;    
        bool setup();
        void save_adoption();
        bool load_adoption();
        void find_ha_instance();
        void write_string(WiFiClient client, const char *str);
};

#endif