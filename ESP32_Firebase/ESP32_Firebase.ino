#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <time.h> // Pour récupérer l'heure via NTP
#include <ESP32Servo.h>

// Définir les informations Wi-Fi
#define WIFI_SSID "Rafed"
#define WIFI_PASSWORD "rafed12345"

// Définir les informations Firebase
#define API_KEY "AIzaSyCS_LkrMcqZxatrXc7HFiHf4cK4QzriT3M"
#define DATABASE_URL "https://smartgrow-4c523-default-rtdb.europe-west1.firebasedatabase.app/"

// Initialisation des objets Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Servo myServo;

// Définir les broches pour les capteurs
#define DHTPIN 26      // GPIO26 pour le DHT11
#define DHTTYPE DHT11  // Type de capteur DHT11
#define SOILPIN 4      // GPIO4 (D2) pour le capteur d'humidité du sol
#define LED_PIN 2      // LED pin
#define FAN_PIN 25  // Define the GPIO pin for the fan
#define WATER_LEVEL_PIN 35  // GPIO pour le capteur de niveau d'eau (analogique)
#define BUZZER_PIN 19  // GPIO utilisée pour le buzzer
#define SERVO_PIN 18
#define PUMP_PIN 26 // Pompe connectée à GPIO 26

DHT dht(DHTPIN, DHTTYPE); // Initialisation du capteur DHT11

// Initialisation du capteur de luminosité VEML7700
Adafruit_VEML7700 veml7700 = Adafruit_VEML7700();

// Variables globales
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// Paramètres NTP (heure réseau)
const char* ntpServer = "pool.ntp.org"; // Serveur NTP
const long gmtOffset_sec = 3600;       // Décalage GMT pour la France (+1h)
const int daylightOffset_sec = 3600;  // Heure d'été (+1h)

// Fonction pour obtenir la date et l'heure actuelles
String getCurrentTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Erreur de récupération de l'heure");
    return "N/A";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Fonction pour lire le niveau d'eau
int readWaterLevel() {
  int waterLevel = analogRead(WATER_LEVEL_PIN); // Lecture de la valeur analogique
  int percentage = map(waterLevel, 0, 4095, 0, 100); // Conversion en pourcentage
  if (percentage < 0) percentage = 0; // Ajuster les valeurs négatives
  if (percentage > 100) percentage = 100; // Ajuster les valeurs au-dessus de 100%
  return percentage;
}
// Fonction pour lire la température
float readTemperature() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    Serial.println("Erreur de lecture de la température !");
    return -1;
  }
  return temp;
}

// Fonction pour lire l'humidité de l'air
float readHumidity() {
  float hum = dht.readHumidity();
  if (isnan(hum)) {
    Serial.println("Erreur de lecture de l'humidité !");
    return -1;
  }
  return hum;
}

// Fonction pour lire l'humidité du sol
int readSoilHumidity() {
  int soilValue = analogRead(SOILPIN); // Lecture de la valeur analogique
  int humidityPercent = map(soilValue, 4095, 0, 0, 100); // Convertir en pourcentage
  return humidityPercent;
}

// Fonction pour lire la luminosité
float readLight() {
  float luminosite = veml7700.readLux();
  if (isnan(luminosite)) {
    Serial.println("Erreur de lecture de la luminosité !");
    return -1;
  }
  return luminosite;
}

void controlLEDandUpdateFirebase(float light, String timestamp) {
    bool isLEDOn = false;

    // Lire la valeur depuis Firebase pour vérifier si l'utilisateur veut forcer l'allumage de la LED
    bool forceLEDOn = false;
    if (Firebase.RTDB.getBool(&fbdo, "/force_LED_on")) {
        forceLEDOn = fbdo.boolData();
        Serial.print("Valeur Firebase force_LED_on : ");
        Serial.println(forceLEDOn ? "true" : "false");
    } else {
        Serial.print("Erreur lors de la lecture de force_LED_on : ");
        Serial.println(fbdo.errorReason());
    }

    if (forceLEDOn) {
        // Allumer la LED si la valeur Firebase est true
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED allumée par commande Firebase.");
        isLEDOn = true;
    } else {
        // Contrôle basé sur la luminosité
        if (isnan(light)) {
            Serial.println("Erreur de lecture de la luminosité !");
        } else {
            Serial.print("Luminosité actuelle: ");
            Serial.println(light);

            if (light < 2) {
                digitalWrite(LED_PIN, HIGH);  // Allumer la LED si la luminosité est inférieure à 2 lux
                Serial.println("LED allumée en raison de la faible luminosité.");
                isLEDOn = true;
            } else {
                digitalWrite(LED_PIN, LOW);  // Éteindre la LED sinon
                Serial.println("LED éteinte.");
            }
        }
    }

    // Mettre à jour Firebase avec l'état actuel de la LED
    if (Firebase.RTDB.setBool(&fbdo, "/lampes_allumees", isLEDOn)) {
        Firebase.RTDB.setString(&fbdo, "/allumage_lampes_time", timestamp);
    } else {
        Serial.print("Erreur de mise à jour Firebase pour l'état de la LED : ");
        Serial.println(fbdo.errorReason());
    }
}

void alertLowWaterLevel() {
    // Activer le buzzer
    digitalWrite(BUZZER_PIN, HIGH);
    
    // Mettre à jour Firebase pour indiquer que le buzzer a été activé
    if (Firebase.RTDB.setBool(&fbdo, "/alerte_buzzer", true)) {
        Serial.println("Firebase mis à jour : alerte_buzzer = true");
    } else {
        Serial.print("Erreur lors de la mise à jour de alerte_buzzer : ");
        Serial.println(fbdo.errorReason());
    }

    delay(500);  // Maintenir pendant 50 ms
}


void controlFanAndUpdateFirebase(float temperature,String timestamp) {
    bool isFanOn = false;

    Serial.print("Température actuelle: ");
    Serial.println(temperature);

    if (temperature > 13) {  // Assuming the fan should turn on above X degrees Celsius
        digitalWrite(FAN_PIN, HIGH);  // Turn on the fan
        Serial.println("Ventilateur allumé en raison de la température élevée.");
        isFanOn = true;
    } else {
        digitalWrite(FAN_PIN, LOW);  // Turn off the fan
        Serial.println("Ventilateur éteint.");
    }

    // Update Firebase with the fan status
    if (Firebase.RTDB.setBool(&fbdo, "/ventilateur_allume", isFanOn)) {
        Firebase.RTDB.setString(&fbdo, "/allumage_lampes_time", timestamp);
    } else {
        Serial.println("Firebase updated with fan status successfully.");
        Serial.println(fbdo.errorReason());
    }
}


void tokenStatusCallback(TokenInfo info) {
  Serial.print("Token Info: type = ");
  Serial.println(info.type);
  
  Serial.print("Status = ");
  Serial.println(info.status);
}

void setup() {
  // Initialisation
  Serial.begin(9600);
  dht.begin(); // Initialiser le capteur DHT
  pinMode(SOILPIN, INPUT); // Configurer la broche du capteur d'humidité du sol en entrée
  pinMode(LED_PIN, OUTPUT);  // Set the LED pin as an output
  pinMode(FAN_PIN, OUTPUT);  // Set the fan pin as an output
  pinMode(BUZZER_PIN, OUTPUT);  // Configure le buzzer comme sortie
  digitalWrite(BUZZER_PIN, LOW); // Assurez-vous que le buzzer est désactivé au démarrage

  myServo.attach(SERVO_PIN);
  myServo.write(0);  // Position initiale fermée

  // Initialisation du capteur VEML7700
  Wire.begin();
  if (!veml7700.begin()) {
    Serial.println("Capteur VEML7700 non détecté !");
    while (1);
  }
  Serial.println("Capteur VEML7700 prêt !");
  
  // Connexion Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion au Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnecté au Wi-Fi");

  // Configuration NTP pour l'heure
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Configuration Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Connexion Firebase réussie");
    signupOK = true;
  } else {
    Serial.printf("Erreur Firebase : %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Récupérer la date et l'heure
    String timestamp = getCurrentTime();

    // Lire la température
    float temperature = readTemperature();
    if (temperature != -1) {
      if (Firebase.RTDB.setFloat(&fbdo, "/temperature", temperature)) {
        Firebase.RTDB.setString(&fbdo, "/temperature_time", timestamp);
        Serial.print("Température envoyée : ");
        Serial.println(temperature);
        controlFanAndUpdateFirebase(temperature,timestamp);
      } else {
        Serial.print("Erreur d'envoi température : ");
        Serial.println(fbdo.errorReason());
      }
    }

    // Lire l'humidité de l'air
    float humidity = readHumidity();
    if (humidity != -1) {
      if (Firebase.RTDB.setFloat(&fbdo, "/humidite", humidity)) {
        Firebase.RTDB.setString(&fbdo, "/humidite_time", timestamp);
        Serial.print("Humidité envoyée : ");
        Serial.println(humidity);
      } else {
        Serial.print("Erreur d'envoi humidité : ");
        Serial.println(fbdo.errorReason());
      }
    }

    // Lire l'humidité du sol
    int soilHumidity = readSoilHumidity();

     if (soilHumidity < 50) {
      digitalWrite(PUMP_PIN, HIGH); // Allumer la pompe
      Serial.println("Pompe activée pour irrigation.");
    } else {
      digitalWrite(PUMP_PIN, LOW); // Éteindre la pompe
      Serial.println("Pompe désactivée. Humidité suffisante.");
    }

    if (Firebase.RTDB.setInt(&fbdo, "/humidite_sol", soilHumidity)) {
      Firebase.RTDB.setString(&fbdo, "/humidite_sol_time", timestamp);
      Serial.print("Humidité du sol envoyée : ");
      Serial.println(soilHumidity);
    } else {
      Serial.print("Erreur d'envoi humidité du sol : ");
      Serial.println(fbdo.errorReason());
    }

    // Lire la luminosité
    float light = readLight();
    if (light != -1) {
      if (Firebase.RTDB.setFloat(&fbdo, "/luminosite", light)) {
        Firebase.RTDB.setString(&fbdo, "/luminosite_time", timestamp);
        Serial.print("Luminosité envoyée : ");
        Serial.println(light);
      } else {
        Serial.print("Erreur d'envoi luminosité : ");
        Serial.println(fbdo.errorReason());
      }
    }
    controlLEDandUpdateFirebase(light,timestamp);

    int waterLevel = readWaterLevel();
    if (Firebase.RTDB.setFloat(&fbdo, "/niveau_eau", ((float)waterLevel))) {
      Firebase.RTDB.setString(&fbdo, "/niveau_eau_time", timestamp);
      Serial.print("Niveau d'eau envoyé : ");
      Serial.print((float)waterLevel);
      Serial.println("%");
      // Vérifier le niveau d'eau et activer le buzzer si nécessaire
      if ((float)waterLevel < 30) {
          Serial.println("Niveau d'eau bas ! Activation du buzzer...");
          alertLowWaterLevel();
    } else {
      Serial.println("Niveau d'eau bon");
      // Désactiver le buzzer
      digitalWrite(BUZZER_PIN, LOW);

      // Mettre à jour Firebase pour indiquer que le buzzer a été activé
      if (Firebase.RTDB.setBool(&fbdo, "/alerte_buzzer", false)) {
          Serial.println("Firebase mis à jour : alerte_buzzer = false");
      } else {
          Serial.print("Erreur lors de la mise à jour de alerte_buzzer : ");
          Serial.println(fbdo.errorReason());
      }
    }
    }else {
      Serial.print("Erreur d'envoi niveau d'eau : ");
      Serial.println(fbdo.errorReason());
    }

    // Lire la valeur de porte_ouverte depuis Firebase
    if (Firebase.RTDB.getBool(&fbdo, "/porte_ouverte")) {
      bool porteOuverte = fbdo.boolData();
      Serial.print("État de la porte : ");
      Serial.println(porteOuverte ? "Ouverte" : "Fermée");

      // Contrôle du servomoteur
      if (porteOuverte) {
        myServo.write(90);  // Ouvrir la porte
        Serial.println("Servomoteur réglé à 90° : Porte ouverte.");
      } else {
        myServo.write(0);   // Fermer la porte
        Serial.println("Servomoteur réglé à 0° : Porte fermée.");
      }
    } else {
      Serial.print("Erreur lors de la lecture de porte_ouverte : ");
      Serial.println(fbdo.errorReason());
    }
}
}
