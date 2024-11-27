#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Ethernet.h>
#include <ESP32Ping.h>
#include <ArduinoJson.h>                          // Pour l'extraction simple des données Json reçues par les requêtes HTTP

//Uncomment to read debug messages
//#define DEBUG

// TODO :
//    Connecter le WIFI avec le bouton WSP de la BOX

// Define NTP Client to get time
const char* ntpServer = "pool.ntp.org";
time_t moment;                               // utilisé pour stocker la date et l'heure au format unix
struct tm loc;                               // Structure qui contient les informations de l'heure et de la date
struct tm loc2;                              // Structure qui contient les informations de l'heure et de la date de dans 2 jours

// System time for sending data
// WIFI connection
const char* HOST_NAME = "EDFTempo";
const char* SSID_NAME = "XXXXXXXXXXXXXXXX";     // WiFi SSID Name
const char* SSID_PASS = "YYYYYYYYYYYYYYYY";     // WiFi SSID Password

volatile int waitMaxWifi = 0;
bool isWIFIconnected = false;

// ******* Valeurs nécessaires à l'accès à l'API "Tempo Like Supply Contract" de RTE *******
// ID Client et ID Secret en base 64, créées sur le site de RTE avec le bouton "Copier en base 64"
#define identificationRTE   "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ=="

const char * idRTE = "Basic " identificationRTE;

// Certificat racine (format PEM) de https://digital.iservices.rte-france.com
const char* root_ca = \
                      "-----BEGIN CERTIFICATE-----\n" \
                      "MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\n" \
                      "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\n" \
                      "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\n" \
                      "MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\n" \
                      "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n" \
                      "hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\n" \
                      "RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\n" \
                      "gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\n" \
                      "KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\n" \
                      "QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\n" \
                      "XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\n" \
                      "DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\n" \
                      "LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\n" \
                      "RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\n" \
                      "jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\n" \
                      "6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\n" \
                      "mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\n" \
                      "Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\n" \
                      "WD9f\n" \
                      "-----END CERTIFICATE-----\n";

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

  disconnectWIFI();
#ifdef DEBUG
  Serial.println("End of setup, starting loop!");
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fonction Lecture Tempo
////////////////////////////////////////////////////////////////////////////////////////////////////////
String errorDescription(int code, HTTPClient& http)
// Liste des codes d'erreurs spécifique à l'API RTE ou message général en clair
{
  switch (code)
  {
    case 400: return "Erreur dans la requête";
    case 401: return "L'authentification a échouée";
    case 403: return "L’appelant n’est pas habilité à appeler la ressource";
    case 413: return "La taille de la réponse de la requête dépasse 7Mo";
    case 414: return "L’URI transmise par l’appelant dépasse 2048 caractères";
    case 429: return "Le nombre d’appel maximum dans un certain laps de temps est dépassé";
    case 509: return "L‘ensemble des requêtes des clients atteint la limite maximale";
    default: break;
  }
  return http.errorToString(code);
}

bool lect_Tempo()
// ****** Deux requêtes vers l'API de RTE, nécessite que le WiFi soit actif ******
{
  int HTTPcode;
  const char* access_token;
  String AccessToken;
  bool requeteOK = true;
  const char* oauthURI =   "https://digital.iservices.rte-france.com/token/oauth/";

  if (WiFi.status() != WL_CONNECTED)
  {
#ifdef DEBUG
    Serial.println("WiFI non disponible. Requête impossible");
#endif
    return false;
  }
  WiFiClientSecure client;     // on passe en connexion sécurisée (par https...)
  HTTPClient http;
  client.setCACert(root_ca);   // permet la connexion sécurisée en vérifiant le certificat racine

  // ************** Première des deux requêtes pour obtenit un token **************
  http.begin(client, oauthURI);

  // Headers nécessaires à la requête
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", idRTE);

  // Send HTTP POST request
  HTTPcode = http.POST(nullptr, 0);

  if (HTTPcode == HTTP_CODE_OK)
  {
    String oauthPayload = http.getString();
#ifdef DEBUG
    Serial.println("------------ Contenu renvoyé par la requête 1 : ------------");
    Serial.println(oauthPayload);
    Serial.println("------------------------------------------------------------\n");
#endif
    StaticJsonDocument<192> doc;
    DeserializationError error = deserializeJson(doc, oauthPayload);
    if (error)     // cas où il y a un problème dans le contenu renvoyé par la requête
    {
#ifdef DEBUG
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
#endif
      access_token = "";
      requeteOK = false;
    }
    else           // cas où le contenu renvoyé par la requête est valide et exploitable
    {
      access_token = doc["access_token"];
      AccessToken = String(access_token);
#ifdef DEBUG
      Serial.print("------------ Access token = ");
      Serial.println(access_token);
      Serial.print("------------ Access token string = ");
      Serial.println(AccessToken);
#endif
    }
  }
  else
  {
#ifdef DEBUG
    Serial.print("erreur HTTP POST: ");
    Serial.println(errorDescription(HTTPcode, http));
#endif
    requeteOK = false;
  }
  http.end();
  if (!requeteOK) return false;

  // ***** Deuxième des deux requêtes pour obtenir la couleur des jours, nécessitant le token *****

  // REMARQUES : l'adresse pour la requête est sous la forme :
  // https://digital.iservices.rte-france.com/open_api/tempo_like_supply_contract/v1/tempo_like_calendars?start_date=2015-06-08T00:00:00%2B02:00&end_date=2015-06-11T00:00:00%2B02:00
  // avec (dans notre cas) "start_date" la date du jour et "end_date" la date du jour + 2
  // Après les "%2B" (signe +) on a le décalage horaire par rapport au temps UTC. On doit obligatoirement avoir "02:00" en heures d'été et "01:00" en heure d'hiver
  // Les heures de début et de fin peuvent rester à "T00:00:00" pour la requête mais doivent être présentes !
  // Pour les mois et les jours, les "0" au début peuvent être omis dans le cas de nombres inférieurs à 10

  String requete = "https://digital.iservices.rte-france.com/open_api/tempo_like_supply_contract/v1/tempo_like_calendars?start_date=";
  requete += String(loc.tm_year + 1900) + "-" + String(loc.tm_mon + 1) + "-" + String(loc.tm_mday) + "T00:00:00%2B0" + String(loc.tm_isdst + 1) + ":00&end_date=" + String(loc2.tm_year + 1900) + "-" + String(loc2.tm_mon + 1) + "-" + String(loc2.tm_mday) + "T00:00:00%2B0" + String(loc2.tm_isdst + 1) + ":00";
  // Remarque : "loc.tm_isdst" est à 0 en heure d'hiver et à 1 en heure d'été

#ifdef DEBUG
  Serial.println("---------------- Adresse pour la requête 2 : ---------------");
  Serial.println(requete);
  Serial.println("------------------------------------------------------------\n");
#endif
  http.begin(client, requete);

  String signalsAutorization = "Bearer " + AccessToken;
#ifdef DEBUG
  Serial.print("------------ signalsAutorization = ");
  Serial.println(signalsAutorization);
#endif
  // Headers nécessaires à la requête
  http.addHeader("Authorization", signalsAutorization);
  http.addHeader("Accept", "application/xml");
  // Mettre la ligne précédente en remarque pour avoir le résultat en json plutôt qu'en xml

  // On envoie la requête HTTP GET
  HTTPcode = http.GET();

  if (HTTPcode == HTTP_CODE_OK)
  {
    String recup = http.getString();              // "recup" est une chaîne de caractères au format xml
#ifdef DEBUG
    Serial.println("------------ Contenu renvoyé par la requête 2 : ------------");
    Serial.println(recup);
    Serial.println("------------------------------------------------------------\n");
#endif
    // Récupération des couleurs
    int posi = recup.indexOf("<Couleur>", 100);   // Recherche de la première occurence de la chaîne "<Couleur>"
    // à partir du 100ème caractère de "recup"
    if (recup.length() > 200)                     // Si la couleur J+1 est connue le String "recup" fait plus de 200 caractères
    {
      CTomorow = (recup.substring(posi + 9, posi + 13)); // Récupération du substring des 4 caractères contenant couleur du lendemain
      // peut être "BLEU", "BLAN" ou "ROUG"
      posi = recup.indexOf("<Couleur>", 230);     // Recherche de la deuxième  occurence de la chaîne "<Couleur>"
      // à partir du 230ème caractère de "recup"
      CToday = (recup.substring(posi + 9, posi + 13)); // Récupération du substring des 4 caractères contenant la couleur du jour
      // peut être "BLEU", "BLAN" ou "ROUG"
    }
    else                                          // cas où la couleur de J+1 n'est pas encore connue
    {
      CToday = (recup.substring(posi + 9, posi + 13)); // Récupération du substring des 4 caractères contenant la couleur du jour
      // peut être "BLEU", "BLAN" ou "ROUG"
      CTomorow = "NON_DEFINI";
    }
#ifdef DEBUG
    Serial.print("Couleur Tempo du jour : "); Serial.println(CToday);
    Serial.print("Couleur Tempo de demain : "); Serial.println(CTomorow + "\n");
#endif
  }
  else
  {
#ifdef DEBUG
    Serial.print("erreur HTTP GET: ");
    Serial.print(HTTPcode);
    Serial.print(" => ");
    Serial.println(errorDescription(HTTPcode, http));
#endif
    requeteOK = false;
  }
  http.end();

  if ((loc.tm_hour >= 6) && (loc.tm_hour < 22))
  {
    // Heure_PLEINE
    HpHc = "HEURE_PLEINE";
  }
  else
  {
    // Heure_CREUSE
    HpHc = "HEURE_CREUSE";
  }

  return requeteOK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//   LOOP
////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
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

    // Définition de la date et de l'heure actuelles
    moment = time(NULL);            // Mise de la date et de l'heure au format unix dans la variable "moment" de type time_m
    loc = *(localtime(&moment));    // Mise dans la structure loc des éléments de date et de l'heure
    // Définition de la date et de l'heure de dans 2 jours (172800 = 2 jours en secondes)
    moment = moment + 172800;       // Mise de la date et de heure de dans 2 jours au format unix dans la variable "moment" de type time_m
    loc2 = *(localtime(&moment));   // Mise dans la structure loc2 des éléments de date et de l'heure de dans 2 jours

    //PreviousSOH = ActualSOH;
    lect_Tempo();

    Serial.println("############################");
    Serial.print("Aujourd'hui = ");
    Serial.print(CToday);
    Serial.print(", demain = ");
    Serial.println(CTomorow);
    Serial.print("Heure pleine/Heure creuse = ");
    Serial.println(HpHc);
    Serial.println("############################");

    // "BLEU", "BLAN" ou "ROUG"
    if (CToday == "BLEU")         {
      analogWrite(LED_TODAY_BLEU, 255 * brightness / 100);
      analogWrite(LED_TODAY_BLANC, 0);
      analogWrite(LED_TODAY_ROUGE, 0);
    }
    else if (CToday == "BLAN")   {
      analogWrite(LED_TODAY_BLEU, 0);
      analogWrite(LED_TODAY_BLANC, 255 * brightness / 100);
      analogWrite(LED_TODAY_ROUGE, 0);
    }
    else if (CToday == "ROUG")   {
      analogWrite(LED_TODAY_BLEU, 0);
      analogWrite(LED_TODAY_BLANC, 0);
      analogWrite(LED_TODAY_ROUGE, 255 * brightness / 100);
    }
    else if (CToday == "NON_DEFINI")  {
      analogWrite(LED_TODAY_BLEU, 0);
      analogWrite(LED_TODAY_BLANC, 0);
      analogWrite(LED_TODAY_ROUGE, 0);
    }

    if (CTomorow == "BLEU")       {
      analogWrite(LED_TOMOR_BLEU, 255 * brightness / 100);
      analogWrite(LED_TOMOR_BLANC, 0);
      analogWrite(LED_TOMOR_ROUGE, 0);
    }
    else if (CTomorow == "BLAN") {
      analogWrite(LED_TOMOR_BLEU, 0);
      analogWrite(LED_TOMOR_BLANC, 255 * brightness / 100);
      analogWrite(LED_TOMOR_ROUGE, 0);
    }
    else if (CTomorow == "ROUG") {
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
#endif

    disconnectWIFI();
  }

  delay(1000 * 300);
}
