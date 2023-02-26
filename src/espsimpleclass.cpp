#include "espsimpleclass.h"

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

ESPSimpleClass:: ESPSimpleClass()
{

}

bool ESPSimpleClass::begin(String device_id, String friendly_name, String model, String sw_version)
{   
    my_device_id = device_id;
    my_sw_version = sw_version;
    my_model = model;
    my_friendly_name = friendly_name;

    return setup();
}

bool ESPSimpleClass::reset_adoption()
{
    Preferences prefs;
    prefs.begin("espsimple");
    prefs.putBool("a", false);
    prefs.putString("hi", "");
    prefs.putString("k", "");
    prefs.end();
    return true;
}

bool ESPSimpleClass::begin(String friendly_name, String model, String sw_version)
{   
    String mac_str = WiFi.macAddress();
    mac_str.replace(":", "");
    mac_str.toUpperCase();

    my_device_id = mac_str;
    my_sw_version = sw_version;
    my_model = model;
    my_friendly_name = friendly_name;

    return setup();
}

bool ESPSimpleClass::setup()
{
    if (!MDNS.begin(my_device_id.c_str()))
    {
        Serial.println("Error setting up MDNS responder!");
        while (1)
        {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");

    // Add service to MDNS-SD
    MDNS.addService("_espsimple", "_tcp", 8901);

    if(load_adoption())
        find_ha_instance();


    server.on("/info", HTTP_GET, [this](AsyncWebServerRequest *request)
              { 
                String info = String("{") +
    "\"device_id\": \"" + my_device_id + "\"," +
    "\"friendly_name\": \"" + my_friendly_name + "\"," +
    "\"model\": \"" + my_model + "\"," +
    "\"sw_version\": \"" + my_sw_version + "\"" +
    +"}";
                request->send(200, "text/json", info); 
                });

    server.on("/adopt", HTTP_POST, [this](AsyncWebServerRequest *request)
    {
        if (!request->hasParam("ha_instance", true)) {
            request->send(400, "text/plain", "bad request");
            return;
        } 
        if (!request->hasParam("key", true)) {
            request->send(400, "text/plain", "bad request");
            return;
        } 
        AsyncWebParameter* ha_instance = request->getParam("ha_instance", true);
        AsyncWebParameter* key = request->getParam("key", true);
        Serial.printf("Adoption request: \n%s\n%s\n", ha_instance->value().c_str(), key->value().c_str());

        ha_hostname = ha_instance->value();
        encryption_key = key->value();
        me_is_adopted = true;

        request->send(200, "text/plain", "adopted"); 

        save_adoption();
        find_ha_instance();

    });

    server.onNotFound(notFound);
    server.begin();

    Serial.println("TCP server started");

    return true;
}

void ESPSimpleClass::find_ha_instance()
{
    int count = MDNS.queryService("_espsmplsrvr", "_tcp");
    for (int i = 0; i < count; i++)
    {
        if (MDNS.hostname(i) == ha_hostname &&
        MDNS.IP(i).toString() != "0.0.0.0")
        {
            ha_ip = MDNS.IP(i);
            ha_port = MDNS.port(i);
            Serial.println("Found HomeAssistant Instance");

            Serial.print("IP address: ");
            Serial.println(ha_ip);

            Serial.print("Port: ");
            Serial.println(ha_port);
            ha_found = true;
            break;
        }
    }
}

void ESPSimpleClass::save_adoption()
{
    Preferences prefs;
    prefs.begin("espsimple");
    prefs.putBool("a", me_is_adopted);
    prefs.putString("hi", ha_hostname);
    prefs.putString("k", encryption_key);
    prefs.end();
}

bool ESPSimpleClass::load_adoption()
{
    Preferences prefs;
    prefs.begin("espsimple");
    me_is_adopted = prefs.getBool("a", false);
    ha_hostname = prefs.getString("hi");
    encryption_key = prefs.getString("k");
    prefs.end();

    return me_is_adopted;
}


void ESPSimpleClass::write_string(WiFiClient client, const char *str)
{
    int len = strlen(str);
    client.write((char *)&len, 4);
    client.write(str, len);
}

bool ESPSimpleClass::send_state(const char *uid, const char *state)
{
    if(!me_is_adopted || !ha_found)
        return false;

    WiFiClient client;

    if (!client.connect(ha_ip, ha_port))
        return false;
    client.write(1);
    write_string(client, my_device_id.c_str());
    write_string(client, uid);
    write_string(client, state);

    client.flush();

    long start = millis();
    while (!client.available() && millis() < start + 1000)
        delay(1);

    bool success = client.read() == 1;
    client.stop();

    return success;
}

bool ESPSimpleClass::register_sensor(const char *uid, const char *name, const char *unit, const char *state_class, const char *device_class)
{
    if(!me_is_adopted || !ha_found)
        return false;

    WiFiClient client;

    if (!client.connect(ha_ip, ha_port))
        return false;

    client.write((uint8_t)0);
    write_string(client, my_device_id.c_str());
    write_string(client, uid);
    write_string(client, name);
    write_string(client, unit);
    write_string(client, state_class);
    write_string(client, device_class);

    client.flush();

    long start = millis();
    while (!client.available() && millis() < start + 1000)
        delay(1);

    bool success = client.read() == 1;
    client.stop();

    return success;
}

ESPSimpleClass ESPSimple;