#!/usr/bin/env python3

import subprocess
import time
import paho.mqtt.client as mqtt

broker = "192.168.0.111"
port = int("1883")
user = "user"
password = "password"

def on_connect(client, userdata, result):
    """Pass publish."""
    pass
    
def on_publish(client, userdata, result):
    """Pass publish."""
    pass

def send_ha_config_message():

    file1 = open('Bluetti_ESP32\Device_EB3A_ha_discover_config.json', 'r')
    lines = file1.readlines()
    
    paho = mqtt.Client("esp32_bluetti_config_ha_discover_py")
    paho.username_pw_set(user, password=password)
    paho.on_connect = on_connect
    paho.connect(broker, port)
    
    count = 0
    # Strips the newline character
    for line in lines:
        try:
            count += 1
            #print("Line{}: {}".format(count, line.strip()))
            l = line.strip()
            #am ersten Blank splitten
            n = l.split(" ", 1)
            #print("-------------------")
            #print(n[0])
            #print("-------------------")
            print(n[1])
            paho.publish(topic=n[0], payload = n[1], qos=1, retain=True,)
        except Exception as e:
            print('An error was produced while processing ' + str(line) + ' with exception: ' + str(e))
            raise
    paho.disconnect()


def on_connect(client, userdata, result):
    """Pass publish."""
    pass
    
def on_publish(client, userdata, result):
    """Pass publish."""
    pass

if __name__ == '__main__':
    
    send_ha_config_message()
