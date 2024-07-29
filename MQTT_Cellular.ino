/*
    Description: 
    Use M5StickC Plus2 and M5 SIM7080G GPS/Cellular combo module
    CAT-M to connect to the MQTT server and implement publishing GPS Location messages.
    Check the status through Serial and Screen. 
    When the MQTT connection is successful, check your MQTT Subscriber
    V1.1 added resets when the cell network doesn't connect. Previously would hang.
*/

#include "M5StickCPlus2.h"
#include "M5_SIM7080G.h"

// USER EDITED FIELDS
String APN = "simbase";	// Your Cell Provider APN
String mqtt_server = "mqtt.example.com";
String mqtt_port   = "1883";
String mqtt_topic  = "Mytopic/subtopic";
String mqtt_user = "mqttuserid";
String mqtt_pass = "mqttpwd";
String assetid   = "1234";	// if you need an ID in the message
const int updateRate = 10;	// How often to send updates (approx!)

String clientid = "M5Stack-"+String(random(0xffff), HEX);

M5_SIM7080G device;
String readstr;

// LOGGING
void log(String str) {
    Serial.print(str);
    StickCP2.Display.clear();
    StickCP2.Display.setCursor(0, 0);
    StickCP2.Display.println(str);
}

// CONVERT Lat/lng to NMEA FORMAT
String decimal_to_nmea(float degrees, int is_latitude) {
  char buf[50];
  
  if (is_latitude){
    degrees = abs(degrees);
    int dd = int(degrees);
    float mm = (degrees - dd) * 60;
    sprintf(buf,"%02d%07.4f", dd,mm);
  }
  else {
    String minus = "";
    if (degrees < 0.0) { minus = "-";}
    degrees = abs(degrees);
    int dd = int(degrees);
    float mm = (degrees - dd) * 60;
    sprintf(buf,"%s%02d%07.4f",minus,dd,mm);
  }
  return((String)buf);
}
String convert_to_nmea( float lat, float lon) {
  String nmea_lat = decimal_to_nmea(lat,1);
  String nmea_lon = decimal_to_nmea(lon,0);

  return(nmea_lat+","+nmea_lon);
}


// WAIT FOR GPS SYNC AND RETURN A MESSAGE
String getGPS() {
  const int max = 30;
  String toks[max];
  String UTC;
  String lat;
  String lng;
  String nmeaTime;
  String data;

  wakeup();
  
  while (1) {
      /* Print GNSS info every 1s */
    String array = device.send_and_getMsg("AT+CGNSINF\r\n");

    int tokCnt = 1;
    int i = array.indexOf(',');
    while (i >= 0 && tokCnt < max) {
      toks[tokCnt] = array.substring(0,i);
      tokCnt++;
      array = array.substring(i + 1);
      i = array.indexOf(',');
    }
    if (tokCnt < max) {
      toks[tokCnt] = array;
      tokCnt++;
    }

    UTC = toks[3];
    lat = toks[4];
    lng = toks[5];
    nmeaTime = UTC.substring(8,14);
    data = String(nmeaTime + "\n" + lat + "\n" + lng + "\n");
    log(data);
    if (lat.toFloat() > 0.0 ) {
      break;
    }
    delay(5000);
  }
  String nmea_coordinates = convert_to_nmea(lat.toFloat(), lng.toFloat());
  // EDIT THIS MESSAGE TO YOUR MQTT MESSAGE FORMAT
  data = String("$GPS,"+String(assetid)+","+nmea_coordinates+","+nmeaTime+",END");
  log(data);
  delay(1000);
  return(data);
}

// TURN ON GPS (cannot run at same time Cell is running)
void gps_power_on() 
{
    log("\nPowering ON GPS..\n");
    while (device.send_and_getMsg("AT+CGNSPWR=1\r\n").indexOf("OK") == -1) {
        delay(1000);
    }

    // Ready to print GPS info 
    log("\nReady for GPS syncing!\n");
    delay(2000);
}

// TURN OFF GPS
void gps_power_off()
{
    log("\nPowering OFF GPS..\n");
    while (device.send_and_getMsg("AT+CGNSPWR=0\r\n").indexOf("OK") == -1) {
        //log("..");
        delay(2000);
    }

}

// ACTIVATE CELL IP NETWORK CONNECTION
int activate_network()
{
      log("ACTIVATING NETWORK..");
      delay(1000);
      // APP Network Active
      device.sendMsg("AT+CNACT=0,1\r\n");
      readstr = device.waitMsg(500);
      log(readstr);
      delay(1000);

      int cnt = 0;
      while(1)
      {
        // Print Network Response IP
        device.sendMsg("AT+CNACT?\r\n");
        readstr = device.waitMsg(500);
        log(readstr);

        if(readstr.indexOf("0,0,") ==-1){
            cnt = 1;
            break;
        }
        if(cnt > 10) {
           cnt = -1;
           log("Failed to get IP address");
           delay(5000);
           ESP.restart();
           break;
        }
        cnt++;
        delay(1000);
      }
      return (cnt);
}

// DEACTIVATE CELL NETWORK (ignore any error msg)
void deactivate_network()
{
      log("DEACTIVATING NETWORK..");
      delay(1000);
      device.sendMsg("AT+SMDISC\r\n");
      delay(1000);
      device.sendMsg("AT+CNACT=0,0\r\n");
      readstr = device.waitMsg(200);
      log(readstr);
      delay(1000);
}

// SEND THE MQTT HEADERS AND MESSAGE
void send_mqtt(String writestr)
{
      log("SENDING MQTT..\n");
      delay(1000);
        // Setup MQTT Connection
        device.sendMsg("AT+SMCONF=\"URL\",\"+mqtt_server+\","+mqtt_port+"\r\n");
        readstr = device.waitMsg(1000);
        log(readstr);

        // MQTT Timeout 
        device.sendMsg("AT+SMCONF=\"KEEPTIME\",60\r\n");
        readstr = device.waitMsg(1000);
        log(readstr);

        // MQTT Clean Session
        device.sendMsg("AT+SMCONF=\"CLEANSS\",1\r\n");
        readstr = device.waitMsg(1000);
        log(readstr);

        // MQTT USERNAME
        device.sendMsg("AT+SMCONF=\"USERNAME\",\"rtMQTT\"\r\n");
        readstr = device.waitMsg(1000);
        log(readstr);

        // MQTT PASSWORD
        device.sendMsg("AT+SMCONF=\"PASSWORD\",\"RT3!MQ$1710\"\r\n");
        readstr = device.waitMsg(1000);
        log(readstr);

        // MQTT Client ID
        device.sendMsg("AT+SMCONF=\"CLIENTID\",\"M5Stack-12345\"\r\n");
        readstr = device.waitMsg(1000);
        log(readstr);

        // MQTT Connection
        device.sendMsg("AT+SMCONN\r\n");
        readstr = device.waitMsg(5000);
        log(readstr);

	// SMCONN can return error even if it actually can send msg(why?)
        //if(readstr.indexOf("ERROR") == -1) {
        //    break;
        //}

        String len = String(writestr.length());
        device.sendMsg("AT+SMPUB=\"+mqtt_topic+\","+len+",1,1\r\n");
        delay(100);
        device.sendMsg(writestr.c_str());
        delay(1000);

}

// WAKE THE SIM7080 MODULE into CMD MODE
void wakeup()
{
     while (device.send_and_getMsg("AT\r\n").indexOf("OK") == -1) {
        log("wakeup()..\n");
        delay(500);
    }
}



// ------
void setup()
{
    M5.begin();
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(3);
    StickCP2.Display.setTextColor(WHITE);
    StickCP2.Display.setTextSize(2);

    log("STARTING UP");
    delay(1000);

    //SIM7080
    device.Init(&Serial2, 33, 32);

     wakeup();
     log("\nReboot SIM7080G..\n");
     while (device.send_and_getMsg("AT+CREBOOT\r\n").indexOf("OK") == -1) {
        //log("..");
        delay(1000);
    }
    delay(5000);
    wakeup();
    gps_power_off();

    // APN MANUAL
    log("\nSETTING UP CELLULAR\n");
    device.sendMsg("AT+CFUN=0\r\n");
    readstr = device.waitMsg(1000);
    log(readstr);
    delay(1000);

    device.sendMsg("AT+CGDCONT=1,\"IP\",\""+APN+"\"\r\n");
    readstr = device.waitMsg(1000);
    log(readstr);
    delay(1000);

    device.sendMsg("AT+CFUN=1\r\n");
    readstr = device.waitMsg(1000);
    log(readstr);
    delay(1000);

    // Check PS
    log("Checking PS Service");
    device.sendMsg("AT+CGATT?\r\n");
    readstr = device.waitMsg(1000);
    log(readstr);
    delay(1000);

    // Check APN
    log("Querying APN Network");
    device.sendMsg("AT+CGNAPN\r\n");
    readstr = device.waitMsg(1000);
    log(readstr);
    delay(1000);

    // SET APN
    device.sendMsg("AT+CNCFG=0,1,\""+APN+"\"\r\n");
    readstr = device.waitMsg(1000);
    log(readstr);
    delay(1000);

    // SIGNAL QUALITY
    log("\nChecking Signal Quality\n");
    int cnt =0;
    while(1){
        device.sendMsg("AT+CSQ\r\n");
        readstr = device.waitMsg(2000);
        log(readstr);
        if(readstr.indexOf("+CSQ: 99,99") ==-1){
            break;
        }
        if (cnt > 10)
        {
          log("No Signal");
          delay(5000);
          ESP.restart();
        }
        cnt++;
    }
    activate_network();
}



void loop()
{
      gps_power_on();
      delay(updateRate*1000);
      String writestr = getGPS();
      gps_power_off();
      if (activate_network() == 1)
      {
        send_mqtt(writestr);
      }
      delay(500);
      //deactivate_network(); // Appears not needed.
      delay(1000);
}
