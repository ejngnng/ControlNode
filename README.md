# This file is for nodemcu arduino project
* implement MQTT and SSDP on nodemcu 

*  Arduino IDE 1.6.8


```
mosquitto version 1.4.8 (build date Fri, 09 Sep 2016 12:39:57 +0200)

mosquitto is an MQTT v3.1 broker.
```

# Feature List
| Feature | Status | Comments |
|:--------|:--------|:---------|
|SSDP	| OK	| 	ssdp client implement		|
|MQTT 	| OK	|	mqtt sub and pub implement		|
|Relay	| OK	|	relay switch control		|
|LED detect| OK	|			|
|Smart config| OK |	config wifi ssid and password with smartpone instead of hard code		|
|OTA 	| [X]	|	update firmware with remote web server		|

# LED Indicator

| Indicator | nodemcu GPIO |
|:----------|:------------|
|SSDP	|D5		|
|MQTT	|D6		|
|RELAY	|D7		|

# Switch 
| device | nodemcu GPIO |
|:-------|:-------|
|Relay1	|D2		|