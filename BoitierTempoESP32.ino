#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Ethernet.h>
#include <ESP32Ping.h>
#include <ArduinoJson.h>                          // Pour l'extraction simple des données Json reçues par les requêtes HTTP
#include <TimeLib.h>                              // Pour la gestion de la date et de l'heure

//Uncomment to read debug messages
//#define DEBUG

// TODO :
//    Verifier la mise à l'heure et le passage heure hiver/ete (NON TESTE):
//    Connecter le WIFI avec le bouton WSP de la BOX
//    Améliorer le server WEB

// Define NTP Client to get time
const char* ntpServer = "pool.ntp.org";
unsigned int Actualmin = 0;
unsigned int Actualsec = 0;
  
//// WEB SERVER ////
WiFiServer server(80);      // Set web server port number to 80

// System time for sending data
// WIFI connection
const char* HOST_NAME = "EDFTempo";
const char* SSID_NAME = "XXXXXXXXXXXXXXXX";     // WiFi SSID Name
const char* SSID_PASS = "YYYYYYYYYYYYYYYY";     // WiFi SSID Password
volatile int waitMaxWifi = 0;
bool isWIFIconnected = false;

/*
  Jblanc=`curl -X 'GET' "https://particulier.edf.fr/services/rest/referentiel/getNbTempoDays?TypeAlerte=TEMPO" -H 'accept: application/json' | jq -r '.PARAM_NB_J_BLANC'`
  Jbleu=`curl -X 'GET' "https://particulier.edf.fr/services/rest/referentiel/getNbTempoDays?TypeAlerte=TEMPO" -H 'accept: application/json' | jq -r '.PARAM_NB_J_BLEU'`
  Jrouge=`curl -X 'GET' "https://particulier.edf.fr/services/rest/referentiel/getNbTempoDays?TypeAlerte=TEMPO" -H 'accept: application/json' | jq -r '.PARAM_NB_J_ROUGE'`

  now=$(date +'%Y-%m-%d')
  #now="2023-02-01"       # BLANC
  #now="2023-02-09"       # ROUGE

  TodayC=`curl -X 'GET' "https://particulier.edf.fr/services/rest/referentiel/searchTempoStore?dateRelevant=$now" -H 'accept: application/json' | jq -r '.couleurJourJ'`
  TomorrowC=`curl -X 'GET' "https://particulier.edf.fr/services/rest/referentiel/searchTempoStore?dateRelevant=$now" -H 'accept: application/json' | jq -r '.couleurJourJ1'`
*/

String CToday;
String CTomorow;
String HpHc;

/*
  Follow this link for the pinout :
    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/_images/esp32-devkitC-v4-pinout.png
*/

#define LED_TODAY_BLEU  12 // ESP32 pin GIOP12 connected to LED
#define LED_TODAY_BLANC 14 // ESP32 pin GIOP14 connected to LED
#define LED_TODAY_ROUGE 27 // ESP32 pin GIOP27 connected to LED

#define LED_TOMOR_BLEU  16 // ESP32 pin GIOP16 connected to LED
#define LED_TOMOR_BLANC 17 // ESP32 pin GIOP17 connected to LED
#define LED_TOMOR_ROUGE 18 // ESP32 pin GIOP18 connected to LED

#define LED_HC 22 // ESP32 pin GIOP33 connected to LED
#define LED_HP 23 // ESP32 pin GIOP33 connected to LED

int brightness = 5;  // how bright the LED is %

///////////////////////////////////////////////////////////////////////////////////////////////////////
// WIFI functions
////////////////////////////////////////////////////////////////////////////////////////////////////////
void disconnectWIFI()
{
  /*
    // Disconnect WIFI
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);

    #ifdef DEBUG
    Serial.println("Wifi disconnected!");
    #endif
    isWIFIconnected = false;
  */
}

bool connectWIFI()
{
  // configure WIFI connection
  waitMaxWifi = 0;

  //WiFi.forceSleepWake();
  WiFi.hostname(HOST_NAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_NAME, SSID_PASS);

  while ( (WiFi.status() != WL_CONNECTED) && (waitMaxWifi < 60) )
  {
    delay(500); // Wait or do something useful
    waitMaxWifi++;
#ifdef DEBUG
    Serial.println("Wifi connecting...");
#endif
  }

  if (waitMaxWifi >= 60)
  {
#ifdef DEBUG
    Serial.println("Wifi not connected!");
#endif
    disconnectWIFI();
    return false;
  }
  else
  {
#ifdef DEBUG
    Serial.println("Wifi connected!");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
#endif
    isWIFIconnected = true;
    return true;
  }
}

bool testWIFI()
{
  // Check if we can connect to domoticz
  bool success = Ping.ping("8.8.8.8", 3);
  if (success)
  {
#ifdef DEBUG
    Serial.println("Init : internet connected");
#endif
    isWIFIconnected = true;
    return true;
  }
  else
  {
#ifdef DEBUG
    Serial.println("Init : internet connection failed");
#endif
    isWIFIconnected = false;
    return false;
  }
}

bool connectandtestWIFI()
{
  if (connectWIFI() == true) return testWIFI();
  else return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// NTP functions
////////////////////////////////////////////////////////////////////////////////////////////////////////
void startNTP()
{
#ifdef DEBUG
  Serial.println("Start NTP");
#endif

  configTzTime("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", ntpServer);
  delay(5000);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setup function
////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  // Configure serial link debug
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  while (!connectandtestWIFI())
  {
    delay(200);
  }

  startNTP();

  // declare pin GIOP33 to be an output:
  pinMode(LED_TODAY_BLEU, OUTPUT);
  pinMode(LED_TODAY_BLANC, OUTPUT);
  pinMode(LED_TODAY_ROUGE, OUTPUT);

  pinMode(LED_TOMOR_BLEU, OUTPUT);
  pinMode(LED_TOMOR_BLANC, OUTPUT);
  pinMode(LED_TOMOR_ROUGE, OUTPUT);

  pinMode(LED_HC, OUTPUT);
  pinMode(LED_HP, OUTPUT);

  analogWrite(LED_TODAY_BLEU, 255);
  analogWrite(LED_TODAY_BLANC, 255);
  analogWrite(LED_TODAY_ROUGE, 255);

  analogWrite(LED_TOMOR_BLEU, 255);
  analogWrite(LED_TOMOR_BLANC, 255);
  analogWrite(LED_TOMOR_ROUGE, 255);

  analogWrite(LED_HC, 255);
  analogWrite(LED_HP, 255);

  delay(2000);

  analogWrite(LED_TODAY_BLEU, 0);
  analogWrite(LED_TODAY_BLANC, 0);
  analogWrite(LED_TODAY_ROUGE, 0);

  analogWrite(LED_TOMOR_BLEU, 0);
  analogWrite(LED_TOMOR_BLANC, 0);
  analogWrite(LED_TOMOR_ROUGE, 0);

  analogWrite(LED_HC, 0);
  analogWrite(LED_HP, 0);

  server.begin();

  disconnectWIFI();
#ifdef DEBUG
  Serial.println("End of setup, starting loop!");
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fonction Lecture Tempo
////////////////////////////////////////////////////////////////////////////////////////////////////////
void lect_Tempo()
{
  String requeteTempo;                                    // pour l'appel au serveur Particulier EDF
  HTTPClient http;
  int httpCode;

  byte month_, jour_, hour_;
  int year_;
  time_t timestamp = time( NULL );
  struct tm *pTime = localtime(&timestamp);

  hour_ = pTime->tm_hour;
  jour_ = pTime->tm_mday;
  month_ = pTime->tm_mon + 1;
  year_ = pTime->tm_year + 1900;

  requeteTempo = ("https://particulier.edf.fr/services/rest/referentiel/searchTempoStore?dateRelevant=");
  requeteTempo += String(year_) + '-' + String(month_) + '-' + String(jour_);

#ifdef DEBUG
  Serial.println("*******************************************");
  Serial.print("Requete Tempo :");          // print a message out in the serial port
  Serial.println(requeteTempo);
  Serial.println(hour_);
  Serial.println("*******************************************");
#endif
  
  http.begin(requeteTempo);
  http.setConnectTimeout(2000);
  httpCode = http.GET();
  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)                       // Vérif que la requête s'est bien passée
    {
      StaticJsonDocument<100> doc2;
      deserializeJson(doc2, (http.getString()));
      CToday   = doc2["couleurJourJ"].as<String>();         // Peut être "TEMPO_BLEU", "TEMPO_BLANC", "TEMPO_ROUGE"
      CTomorow = doc2["couleurJourJ1"].as<String>();       // Idem ci-dessus
      // le ".as<String>()" à la fin des requêtes Json transforme la chaîne de caractère du résultat en string
      // bien plus facile à manipuler
    }
  }
  http.end();

  if ((hour_ >= 6) && (hour_ < 22))
  {
    // Heure_PLEINE
    HpHc = "HEURE_PLEINE";
  }
  else
  {
    // Heure_CREUSE
    HpHc = "HEURE_CREUSE";
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//   WEB SERVER
////////////////////////////////////////////////////////////////////////////////////////////////////////
void webserver()
{
  WiFiClient client = server.available();   // Listen for incoming clients

#ifdef DEBUG
  Serial.println("Start web server...");          // print a message out in the serial port
#endif

  if (client)                              // If a new client connects,
  {
#ifdef DEBUG
    Serial.println("New Client.");          // print a message out in the serial port
#endif

    while (client.connected())
    { // loop while the client's connected
      if (client.available())              // if there's bytes to read from the client,
      {
        char c = client.read();             // read a byte, then
#ifdef DEBUG
        Serial.write(c);                    // print it out the serial monitor
#endif

        if (c == '\n') {                    // if the byte is a newline character

          lect_Tempo();

          // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
          // and a content-type so the client knows what's coming, then a blank line:
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();

          // Display the HTML web page
          client.println("<!DOCTYPE html><html>");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<link rel=\"icon\" href=\"data:,\">");
          // CSS to style the on/off buttons
          // Feel free to change the background-color and font-size attributes to fit your preferences
          client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }");
          client.println(".button { background-color: #FF0000; border: black; color: white; padding: 16px 120px;");         // Rouge

          client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
          client.println(".button2 {background-color: #0000FF; border: black; color: white; padding: 16px 120px;");                                                // Bleu

          client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
          client.println(".button4 {background-color: #00FF00; border: black; color: black; padding: 16px 120px;");                                                // Vert

          client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
          client.println(".button5 {background-color: #ff8000; border: black; color: black; padding: 16px 120px;");                                                // Orange

          client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
          client.println(".button6 {background-color: #BDBDBB; border: black; color: white; padding: 16px 120px;");                                                // Gris

          client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
          client.println(".button3 {background-color: #FFFFFF; border: black; color: black; padding: 16px 120px;} </style></head>");                               // Blanc

          // Web Page Heading
          client.println("<body><h1>EDF TEMPO Web Server</h1>");

          if (CToday == "TEMPO_BLEU")
          {
            client.println("<p><a href=\\><button class=\"button button2\">TODAY</button></a></p>");
          }
          else if (CToday == "TEMPO_BLANC")
          {
            client.println("<p><a href=\\><button class=\"button button3\">TODAY</button></a></p>");
          }
          else if (CToday == "TEMPO_ROUGE")
          {
            client.println("<p><a href=\\><button class=\"button\">TODAY</button></a></p>");
          }
          else if (CToday == "NON_DEFINI")
          {
            client.println("<p><a href=\\><button class=\"button button6\">TODAY</button></a></p>");
          }

          if (CTomorow == "TEMPO_BLEU")
          {
            client.println("<p><a href=\\><button class=\"button button2\">TOMORROW</button></a></p>");
          }
          else if (CTomorow == "TEMPO_BLANC")
          {
            client.println("<p><a href=\\><button class=\"button button3\">TOMORROW</button></a></p>");
          }
          else if (CTomorow == "TEMPO_ROUGE")
          {
            client.println("<p><a href=\\><button class=\"button\">TOMORROW</button></a></p>");
          }
          else if (CTomorow == "NON_DEFINI")
          {
            client.println("<p><a href=\\><button class=\"button button6\">TOMORROW</button></a></p>");
          }

          if (HpHc == "HEURE_PLEINE")
          {
            client.println("<p><a href=\\><button class=\"button button5\">HEURE PLEINE</button></a></p>");
          }
          else if (HpHc == "HEURE_CREUSE")
          {
            client.println("<p><a href=\\><button class=\"button button4\">HEURE CREUSE</button></a></p>");
          }

          client.println("</body></html>");
          // The HTTP response ends with another blank line
          client.println();
          // Break out of the while loop
          break;
        }
      }
    }

    // Close the connection
    client.stop();
#ifdef DEBUG
    Serial.println("Client disconnected.");
    Serial.println("");
#endif
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//   LOOP
////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  if ((connectWIFI() == true))
  {
    webserver();
  }

  // Envoie des données toutes les 5 minutes
  time_t timestamp = time( NULL );
  struct tm *pTime = localtime(&timestamp );

  // Envoie des SOH toutes les 5 minutes
  if ((Actualmin % 5 == 0) && (Actualsec == 0))
  {
    if ((connectWIFI() == true))
    {
      //timeClient.update();
#ifdef DEBUG
      Serial.println("############################");
#define MAX_SIZE 80
      time_t timestamp = time( NULL );
      char buffer[MAX_SIZE];
      struct tm *pTime = localtime(&timestamp );
      strftime(buffer, MAX_SIZE, "%d/%m/%Y %H:%M:%S", pTime);
      Serial.println(buffer);
#endif

      //PreviousSOH = ActualSOH;
      lect_Tempo();

#ifdef DEBUG
      Serial.println("############################");
      Serial.print("Aujourd'hui = ");
      Serial.print(CToday);
      Serial.print(", demain = ");
      Serial.println(CTomorow);
      Serial.print("Heure pleine/Heure creuse = ");
      Serial.println(HpHc);
      Serial.println("############################");
#endif

      if (CToday == "TEMPO_BLEU")         {
        analogWrite(LED_TODAY_BLEU, 255 * brightness / 100);
        analogWrite(LED_TODAY_BLANC, 0);
        analogWrite(LED_TODAY_ROUGE, 0);
      }
      else if (CToday == "TEMPO_BLANC")   {
        analogWrite(LED_TODAY_BLEU, 0);
        analogWrite(LED_TODAY_BLANC, 255 * brightness / 100);
        analogWrite(LED_TODAY_ROUGE, 0);
      }
      else if (CToday == "TEMPO_ROUGE")   {
        analogWrite(LED_TODAY_BLEU, 0);
        analogWrite(LED_TODAY_BLANC, 0);
        analogWrite(LED_TODAY_ROUGE, 255 * brightness / 100);
      }
      else if (CToday == "NON_DEFINI")  {
        analogWrite(LED_TODAY_BLEU, 0);
        analogWrite(LED_TODAY_BLANC, 0);
        analogWrite(LED_TODAY_ROUGE, 0);
      }

      if (CTomorow == "TEMPO_BLEU")       {
        analogWrite(LED_TOMOR_BLEU, 255 * brightness / 100);
        analogWrite(LED_TOMOR_BLANC, 0);
        analogWrite(LED_TOMOR_ROUGE, 0);
      }
      else if (CTomorow == "TEMPO_BLANC") {
        analogWrite(LED_TOMOR_BLEU, 0);
        analogWrite(LED_TOMOR_BLANC, 255 * brightness / 100);
        analogWrite(LED_TOMOR_ROUGE, 0);
      }
      else if (CTomorow == "TEMPO_ROUGE") {
        analogWrite(LED_TOMOR_BLEU, 0);
        analogWrite(LED_TOMOR_BLANC, 0);
        analogWrite(LED_TOMOR_ROUGE, 255 * brightness / 100);
      }
      else if (CTomorow == "NON_DEFINI")  {
        analogWrite(LED_TOMOR_BLEU, 0);
        analogWrite(LED_TOMOR_BLANC, 0);
        analogWrite(LED_TOMOR_ROUGE, 0);
      }

      if (HpHc == "HEURE_PLEINE")         {
        analogWrite(LED_HC, 0);
        analogWrite(LED_HP, 255 * brightness / 100);
      }
      else if (HpHc == "HEURE_CREUSE")    {
        analogWrite(LED_HC, 255 * brightness / 100);
        analogWrite(LED_HP, 0);
      }

#ifdef DEBUG
      Serial.print("Disconnect WIFI and enter light sleep for ");
      Serial.println("s.");
      //delay(1000);
#endif
      disconnectWIFI();
    }
  }

  Actualmin = pTime->tm_min;
  Actualsec = pTime->tm_sec;
  delay(1000);
}
