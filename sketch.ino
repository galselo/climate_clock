/* THE USE OF THIS SKETCH IS AT THE YOUR SOLE RISK. 
 * THE ENTIRE RISK AS TO SATISFACTORY QUALITY, AND PERFORMANCE IS WITH YOU, 
 * THE USER OF THIS SKETCH. THIS SKETCH IS PROVIDED "AS IS" WITH ALL FAULTS AND 
 * WITHOUT WARRANTY OF ANY KIND. NO USE OF THIS SKETCH IS AUTHORIZED EXCEPT THE 
 * INTENDED ONE.
 */

// oled screen
#include <U8g2lib.h>

// esp
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

// wifi client
#include <WiFiClient.h>

// eps8266 variable
ESP8266WiFiMulti WiFiMulti;

// oled connections
// clock/d0, data/d1, cs, dc, reset
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, D4, D3, D0, D1, D2);

unsigned long tlast = 0; // init time of the last url retriving (millis)
unsigned long tlast_what = 0; // init time of the last screen change (millis)
int what = 0;  // what to display
int what_max = 5; // max number of display types
String data[6]; // data fields from url
String api_url = "http://smanettare.altervista.org/climate_clock/get_data.php";

// *************
void setup() {

  // init serial
  Serial.begin(9600);

  // init OLED
  u8g2.begin();

  printOLED("connecting...");
  // wait to start
  for (int i = 0; i < 4; i++) {
    delay(1000);
  }

  // set wifi mode and connect
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("YOUR_SSID", "YOUR_PASSWORD");
  Serial.println("AP added");

}

// **************
void loop() {

  // download data every minute
  if (millis() - tlast > 60000) {
    read_from_server(data);
    tlast = millis(); // store loading time
  }

  // change screen type every 10s
  if (millis() - tlast_what > 10000) {
    what = (what + 1) % what_max;
    tlast_what = millis(); // store loading time
  }

  // do stuff and update oled
  manage(data);

  // refresh time
  delay(50);
}

// ********
// manage retrived data and write to oled
void manage(String data[]) {

  // time left for specific temperature
  int time_left15C = data[0].toInt();
  int time_left20C = data[1].toInt();

  // carbon left for specific temperature, tonn
  double carbon_left15C = data[2].toDouble();
  double carbon_left20C = data[3].toDouble();

  // carbon loss rate, tonn/s
  double loss_rate = data[4].toDouble();

  String sentence = data[5];

  Serial.println(sentence);

  // prepare data for oled depending on screen type
  String message[4];
  String mm;
  if (what == 0) {
    mm = cb2string(carbon_left15C, loss_rate);
    message[0] = "Per 1.5 gradi";
  } else if (what == 1) {
    mm = utime2string(time_left15C);
    message[0] = "ovvero";
  } else if (what == 2) {
    mm = cb2string(carbon_left20C, loss_rate);
    message[0] = "Per 2.0 gradi";
  } else if (what == 3) {
    mm = utime2string(time_left20C);
    message[0] = "ovvero";
  } else if (what == 4) {
    message[0] = split_str(sentence, ';', 0);
    message[1] = split_str(sentence, ';', 1);
    message[2] = split_str(sentence, ';', 2);
    message[3] = split_str(sentence, ';', 3);
  } else {
    mm = "ERROR";
  }

  // write to oled
  if (what < 4) {
    message[1] = split_str(mm, ';', 0);
    message[2] = split_str(mm, ';', 1);
    message[3] = split_str(mm, ';', 2);
  }
  printOLEDlines(message);

}

// ********************
// covert carbon budget to oled message (NOTE: semicolons are new lines in oled)
String cb2string(double cb, double loss) {
  int dt = millis() - tlast;
  return "rimangono;" + eng(floor(cb - dt * loss)) + ";tonn di CO2";
}

// *************
// add thousand separators (NOTE: only works for this specific project!)
String eng(double uu) {
  String us = split_str(String(uu), '.', 0);
  String es = "";
  for (int i = 0; i < us.length(); i++) {
    int j = us.length() - i - 1;
    es = us.substring(j, j + 1) + es;
    if (i % 3 == 2 && i < 9) {
      es = "." + es;
    }
  }
  return es;
}


// ********************
// convert time in seconds to oled string (NOTE: semicolons are new lines in oled)
String utime2string(int uu) {
  float fs[6] = {365.25 * 3600. * 24., 365.25 * 3600. * 24. / 12., 24.*3600., 3600., 60., 1.};
  String ns[6] = {"anni ", "mesi;", "giorni ", "ore;", "minuti ", "sec"};
  int u = uu - round((millis() - tlast) / 1e3);
  String s = "";
  for (int i = 0; i < 6; i++) {
    int t = floor(u / fs[i]);
    u -= floor(fs[i] * t);
    s += String(t) + " " + ns[i];
  }
  return s;
}

// ********************
// read data from server
String *read_from_server(String data[6]) {

  for (int i = 0; i < 6; i++) {
    data[i] = "";
  }

  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    // set client
    HTTPClient http;
    Serial.println("[HTTP] begin...");

    // read client
    if (http.begin(client, api_url)) { // HTTP

      Serial.println("[HTTP] GET...");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          // read page content
          String payload = http.getString();
          Serial.println("xxx" + payload + "yyy");
          for (int i = 0; i < 6; i++) {
            data[i] = split_str(payload, '|', i);
          }
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.println("[HTTP} Unable to connect");
    }
  }
  return data;
}


//***************
void printOLED(String message) {
  printOLEDxy(message, 0, 16);
}

//***************
void printOLEDxy(String message, int xx, int yy) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g_font_unifont);
    u8g2.setCursor(xx, yy);
    u8g2.print(message);
  }
  while (u8g2.nextPage());
}


//***************
void printOLEDlines(String message[]) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g_font_unifont);
    for (int i = 0; i < 4; i++) {
      u8g2.setCursor(0, 16 * (i + 1));
      u8g2.print(message[i]);
    }
  }
  while (u8g2.nextPage());
}

// **********************
// split string function, see
// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String split_str(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
