#include "BluettiConfig.h"
#include "MQTT.h"
#include "BWifi.h"
#include "BTooth.h"
#include "utils.h"

#include <WiFi.h>
#include <PubSubClient.h>

#include "Servo.h"


WiFiClient mqttClient;  
PubSubClient client(mqttClient);
int publishErrorCount = 0;
unsigned long lastMQTTMessage = 0;
unsigned long previousDeviceStatePublish = 0;
unsigned long previousDeviceStateStatusPublish = 0;

String map_field_name(enum field_names f_name){
   switch(f_name) {
      case DC_OUTPUT_POWER:
        return "dc_output_power";
        break; 
      case AC_OUTPUT_POWER:
        return "ac_output_power";
        break; 
      case DC_OUTPUT_ON:
        return "dc_output_on";
        break; 
      case AC_OUTPUT_ON:
        return "ac_output_on";
        break; 
      case POWER_GENERATION:
        return "power_generation";
        break;       
      case TOTAL_BATTERY_PERCENT:
        return "total_battery_percent";
        break; 
      case DC_INPUT_POWER:
        return "dc_input_power";
        break;
      case AC_INPUT_POWER:
        return "ac_input_power";
        break;
      case AC_INPUT_VOLTAGE:
        return "ac_input_voltage";
        break;
      case INTERNAL_PACK_VOLTAGE:
        return "internal_pack_voltage";
        break;
      case SERIAL_NUMBER:
        return "serial_number";
        break;
      case ARM_VERSION:
        return "arm_version";
        break;
      case DSP_VERSION:
        return "dsp_version";
        break;
      case DEVICE_TYPE:
        return "device_type";
        break;
      case UPS_MODE:
        return "ups_mode";
        break;
      case AUTO_SLEEP_MODE:
        return "auto_sleep_mode";
        break;
      case GRID_CHANGE_ON:
        return "grid_change_on";
        break;
      case INTERNAL_AC_VOLTAGE:
        return "internal_ac_voltage";
        break;
      case INTERNAL_AC_FREQUENCY:
        return "internal_ac_frequency";
        break;
      case INTERNAL_CURRENT_ONE:
        return "internal_current_one";
        break;
      case PACK_NUM_MAX:
        return "pack_max_num";
        break;
      case INTERNAL_DC_INPUT_VOLTAGE:
        return "internal_dc_input_voltage";
        break;
      case LED_MODE:
        return "led_mode";
        break;
      case POWER_OFF:
        return "power_off";
        break;
      case ECO_ON:
        return "eco_on";
        break;
      case ECO_SHUTDOWN:
        return "eco_shutdown";
        break;
      case CHARGING_MODE:
        return "charging_mode";
        break;
      case POWER_LIFTING_ON:
        return "power_lifting_on";
        break;
      case POWER_ON_BY_SERVO:
        return "power_on_by_servo";
        break;
      default:
        return "unknown";
        break;
   }
  
}

// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String topic_path = String(topic);
  
  Serial.print("MQTT Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(" Payload: ");
  String strPayload = String((char * ) payload);
  Serial.println(strPayload);

  bt_command_t command;
  command.prefix = 0x01;
  command.field_update_cmd = 0x06;

  for (int i=0; i< sizeof(bluetti_device_command)/sizeof(device_field_data_t); i++){
      if (topic_path.indexOf(map_field_name(bluetti_device_command[i].f_name)) > -1){
            command.page = bluetti_device_command[i].f_page;
            command.offset = bluetti_device_command[i].f_offset;
            
            //Quick&Dirty (FIXME): map ON, OFF, LOW, HIGH, SOS, ... to numeric values for the command to send by BL
            //e.g. for "power_off" from "ON" to "1"
			      String current_name = map_field_name(bluetti_device_command[i].f_name);
            if(current_name == "power_off") {
                  if (strPayload == "ON") {
                    strPayload = "1";
                  }
            } 
            else if(current_name == "led_mode") {
                  if (strPayload == "LED_LOW") {
                    strPayload = "1";
                  }
                  else if (strPayload == "LED_HIGH") {
                    strPayload = "2";
                  }
                  else if (strPayload == "LED_SOS") {
                    strPayload = "3";
                  }
                  else if (strPayload == "LED_OFF") {
                    strPayload = "4";
                  }
            } 
            else if(current_name == "eco_shutdown") {
                  if (strPayload == "ONE_HOUR") {
                    strPayload = "1";
                  }
                  else if (strPayload == "TWO_HOURS") {
                    strPayload = "2";
                  }
                  else if (strPayload == "THREE_HOURS") {
                    strPayload = "3";
                  }
                  else if (strPayload == "FOUR_HOURS") {
                    strPayload = "4";
                  }
            } 
            else if(current_name == "charging_mode") {
                  if (strPayload == "STANDARD") {
                    strPayload = "0";
                  }
                  else if (strPayload == "SILENT") {
                    strPayload = "1";
                  }
                  else if (strPayload == "TURBO") {
                    strPayload = "2";
                  }
			      }
            else if(current_name == "ac_output_on") {
                  if (strPayload == "ON") {
                    strPayload = "1";
                  }
                  else if (strPayload == "OFF") {
                    strPayload = "0";
                  }
            }
            else if(current_name == "dc_output_on") {
                  if (strPayload == "ON") {
                    strPayload = "1";
                  }
                  else if (strPayload == "OFF") {
                    strPayload = "0";
                  }
            }
            else if(current_name == "eco_on") {
                  if (strPayload == "ON") {
                    strPayload = "1";
                  }
                  else if (strPayload == "OFF") {
                    strPayload = "0";
                  }
            }
            else if(current_name == "power_lifting_on") {
                  if (strPayload == "ON") {
                    strPayload = "1";
                  }
                  else if (strPayload == "OFF") {
                    strPayload = "0";
                  }
            }

    }
  }
  Serial.print(" Payload - switched: ");
  Serial.println(strPayload);

  command.len = swap_bytes(strPayload.toInt());
  command.check_sum = modbus_crc((uint8_t*)&command,6);
  lastMQTTMessage = millis();
  
  //if topic end with by_servo, we do not sent BT, we send to Servo at GPIO
  if (topic_path.endsWith("by_servo")) {
     deviceServoPress(strPayload.toInt());
  }
  else
  {
     sendBTCommand(command);
  }
}

void subscribeTopic(enum field_names field_name) {
#ifdef DEBUG
  Serial.println("subscribe to topic: " +  map_field_name(field_name));
#endif
  char subscribeTopicBuf[512];
  ESPBluettiSettings settings = get_esp32_bluetti_settings();

  sprintf(subscribeTopicBuf, "bluetti/%s/command/%s", settings.bluetti_device_id, map_field_name(field_name).c_str() );
  client.subscribe(subscribeTopicBuf);
  lastMQTTMessage = millis();

}

void publishTopic(enum field_names field_name, String value){
  char publishTopicBuf[1024];
 
#ifdef DEBUG
  Serial.println("publish topic for field: " +  map_field_name(field_name));
#endif
  
  //sometimes we get empty values / wrong vales - all the time device_type is empty
  if (map_field_name(field_name) == "device_type" && value.length() < 3){

    Serial.println(F("Error while publishTopic! 'device_type' can't be empty, reboot device)"));
    ESP.restart();
   
  } 

  ESPBluettiSettings settings = get_esp32_bluetti_settings();
  sprintf(publishTopicBuf, "bluetti/%s/state/%s", settings.bluetti_device_id, map_field_name(field_name).c_str() ); 
  lastMQTTMessage = millis();
  if (!client.publish(publishTopicBuf, value.c_str() )){
    publishErrorCount++;
    AddtoMsgView(String(lastMQTTMessage) + ": publish ERROR! " + map_field_name(field_name) + " -> " + value);
  }
  else{
    AddtoMsgView(String(lastMQTTMessage) + ": " + map_field_name(field_name) + " -> " + value);
  }
  
  
 
}

void publishDeviceState(){
  char publishTopicBuf[1024];

  ESPBluettiSettings settings = get_esp32_bluetti_settings();
  sprintf(publishTopicBuf, "bluetti/%s/state/%s", settings.bluetti_device_id, "device" ); 
  String value = "{\"IP\":\"" + WiFi.localIP().toString() + "\", \"MAC\":\"" + WiFi.macAddress() + "\", \"Uptime\":" + millis() + "}"; 
  if (!client.publish(publishTopicBuf, value.c_str() )){
    publishErrorCount++;
  }
  lastMQTTMessage = millis();
  previousDeviceStatePublish = millis();
  
}

void publishDeviceStateStatus(){
  char publishTopicBuf[1024];

  ESPBluettiSettings settings = get_esp32_bluetti_settings();
  sprintf(publishTopicBuf, "bluetti/%s/state/%s", settings.bluetti_device_id, "device_status" ); 
  String value = "{\"MQTTconnected\":" + String(isMQTTconnected()) + ", \"BTconnected\":" + String(isBTconnected()) + "}"; 
  if (!client.publish(publishTopicBuf, value.c_str() )){
    publishErrorCount++;
  }
  lastMQTTMessage = millis();
  previousDeviceStateStatusPublish = millis();
 
}

void deviceServoPress(int degree){
     Serial.println("triggerButtonPress: " +  degree);
     triggerButtonPress(degree);
}

void initMQTT(){

    enum field_names f_name;
    ESPBluettiSettings settings = get_esp32_bluetti_settings();
    Serial.print("Connecting to MQTT at: ");
    Serial.print(settings.mqtt_server);
    Serial.print(":");
    Serial.println(F(settings.mqtt_port));
    
    client.setServer(settings.mqtt_server, atoi(settings.mqtt_port));
    client.setCallback(callback);

    bool connect_result;
    const char connect_id[] = "Bluetti_ESP32";
    if (settings.mqtt_username) {
        connect_result = client.connect(connect_id, settings.mqtt_username, settings.mqtt_password);
    } else {
        connect_result = client.connect(connect_id);
    }
    
    if (connect_result) {
        
      Serial.println(F("Connected to MQTT Server... "));

      // subscribe to topics for commands
      for (int i=0; i< sizeof(bluetti_device_command)/sizeof(device_field_data_t); i++){
        subscribeTopic(bluetti_device_command[i].f_name);
      }

      publishDeviceState();
      publishDeviceStateStatus();
      
      publishHAConfig();
    }    
};

void handleMQTT(){
    if ((millis() - lastMQTTMessage) > (MAX_DISCONNECTED_TIME_UNTIL_REBOOT * 60000)){ 
      Serial.println(F("MQTT is disconnected over allowed limit, reboot device"));
      ESP.restart();
    }
      
    if ((millis() - previousDeviceStatePublish) > (DEVICE_STATE_UPDATE * 60000)){ 
      publishDeviceState();
    }
    if ((millis() - previousDeviceStateStatusPublish) > (DEVICE_STATE_STATUS_UPDATE * 60000)){ 
      publishDeviceStateStatus();
    }
    if (!isMQTTconnected() && publishErrorCount > 5){
      Serial.println(F("MQTT lost connection, try to reconnect"));
      client.disconnect();
      lastMQTTMessage=0;
      previousDeviceStatePublish=0;
      previousDeviceStateStatusPublish=0;
      publishErrorCount=0;
      AddtoMsgView(String(millis()) + ": MQTT connection lost, try reconnect");
      initMQTT();

    }
    
    client.loop();
}

bool isMQTTconnected(){
    if (client.connected()){
      return true;
    }
    else
    {
      return false;
    }  
}

int getPublishErrorCount(){
    return publishErrorCount;
}
unsigned long getLastMQTTMessageTime(){
    return lastMQTTMessage;
}
unsigned long getLastMQTDeviceStateMessageTime(){
    return previousDeviceStatePublish;
}
unsigned long getLastMQTDeviceStateStatusMessageTime(){
    return previousDeviceStateStatusPublish;
}


//FIXME (later): for HA Discovery the config is send externaly because for changes no need to recompile
//external python: Device_EB3A_ha_discover_config.py with customized input in Device_EB3A_ha_discover_config.json
void publishHAConfig(){
  
        Serial.println("Hint: When HA MQTT Discovery Config is needed, please use external python (Device_EB3A_ha_discover_config.py)");
  
}
