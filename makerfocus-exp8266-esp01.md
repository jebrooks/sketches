To flash:
Use the betemcu.cn board, not the red board with 'pwrenten' on it

Board Managr -> esp8266 v2.5.2

1. H_PD (en) high, GPIO0 low, RST low
2. Power on
3. disconnect RST
4. flash as generic esp8266.

Run mode: en high; float rst, gpio0, and gpio2

v9.2 and 9.3.1 of tasmota had fluky behavior after connecting to local wifi.  v7.2 worked better.
