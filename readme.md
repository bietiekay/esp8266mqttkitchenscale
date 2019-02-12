ESP8266 Kitchen Scale - (C) Daniel Kirstenpfad 2018 - 2019

http://www.schrankmonster.de

This includes the arduino source code + HX711 library as well as the web application.

The ESP8266 arduino application tries to connect to wifi and then to an mqtt broker.

It'll send the weight measured to the mqtt stream.

The web application will require websockets on the MQTT broker to work and will try to
connect to the mqtt broker that way.


