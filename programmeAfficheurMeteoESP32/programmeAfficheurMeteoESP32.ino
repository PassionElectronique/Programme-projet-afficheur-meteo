// ===================================================================================================
//   ______               _                  _///_ _           _                   _
//  /   _  \             (_)                |  ___| |         | |                 (_)
//  |  [_|  |__  ___  ___ _  ___  _ __      | |__ | | ___  ___| |_ _ __ ___  _ __  _  ___  _   _  ___
//  |   ___/ _ \| __|| __| |/ _ \| '_ \_____|  __|| |/ _ \/  _|  _| '__/   \| '_ \| |/   \| | | |/ _ \
//  |  |  | ( ) |__ ||__ | | ( ) | | | |____| |__ | |  __/| (_| |_| | | (_) | | | | | (_) | |_| |  __/
//  \__|   \__,_|___||___|_|\___/|_| [_|    \____/|_|\___|\____\__\_|  \___/|_| |_|_|\__  |\__,_|\___|
//                                                                                      | |
//                                                                                      \_|
// ===================================================================================================
//
//  Projet  :       Afficheur météo ESP32
//  Fichier :       programmeAfficheurMeteoESP32.ino
//  Auteur  :       Jérôme TOMSKI
//  Créé le :       20.01.2026
//  Site    :       https://passionelectronique.fr/
//  GitHub  :       https://github.com/PassionElectronique/
//  Licence :       https://creativecommons.org/licenses/by-nc-nd/4.0/deed.fr (BY-NC-ND 4.0 CC)
//
// ===================================================================================================
//
//  ---------
//  Remarques
//  ---------
//
//    1)  Penser à configurer le fichier "User_Setup.h" (situé dans le répertoire de la librairie TFT_eSPI),
//        avec le contenu du fichier joint ici (nommé "User_Setup.h.pour.information")
//
//    2)  Penser à installer les librairies "ArduinoJson" et "TFT_eSPI", si c'est pas déjà fait !
//
// ===================================================================================================

// Inclusion des librairies dont nous aurons besoin ici
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

// Inclusion des images de fond météo
#include "1-fond-ensoleille.h"
#include "2-fond-nuageux.h"
#include "3-fond-couvert.h"
#include "4-fond-brouillard.h"
#include "5-fond-pluvieux.h"
#include "6-fond-neigeux.h"
#include "7-fond-orageux.h"

// Paramètres de cette application
#define LATITUDE_A_ANALYSER "44.823462" // Exemple pris ici : "Gare de Bordeaux Saint Jean"
#define LONGITUDE_A_ANALYSER "-0.556514"
#define TIMEZONE_CIBLEE "Europe/Paris"             // Nota : c'est juste pour que l'API retourne l'heure locale ciblée, et non l'heure GMT simple
#define WIFI_SSID "METTRE_LE_SSID_DE_SON_WIFI_ICI" // Par exemple : "livebox-xxxx"
#define WIFI_PASS "METTRE_LA_CLEF_DE_SON_WIFI_ICI" // Nota : sans espace aucun !
#define DELAI_RAFRAICHISSEMENT_INFOS_METEO_EN_MS 600'000
// Nota : 600'000 secondes = 10 minutes (donc un rafraîchissement toutes les 10 minutes ; du moins, dans les limites de ce que propose cette API météo gratuite !

// Constantes (autres)
#define TEMPS_ENSOLEILLE 1
#define TEMPS_NUAGEUX 2
#define TEMPS_COUVERT 3
#define TEMPS_BROUILLARD 4
#define TEMPS_PLUVIEUX 5
#define TEMPS_NEIGEUX 6
#define TEMPS_ORAGEUX 7
#define TEMPS_INCONNU 8

// Création d'une instance de TFT_eSPI
TFT_eSPI tft;

// Variables
uint16_t couleur_texte_temperature_sur_16bits;
uint16_t couleur_texte_temps_actuel_sur_16bits;
uint16_t couleur_ligne_separatrice_sur_16bits;
uint16_t couleur_texte_du_bas_sur_16bits;
uint8_t temps_actuel;
String ligne_1_decrivant_le_temps_actuel;
String ligne_2_decrivant_le_temps_actuel;
String endpoint_API_meteo;


// ========================
// Initialisation programme
// ========================
void setup() {

  delay(3000);

  // ------------------------
  // Initialisation écran TFT
  // ------------------------
  tft.init();
  tft.setSwapBytes(true);    // Permet de corriger le "bug" de décalage de couleurs
  tft.setRotation(3);        // Orientation (3 = "paysage, inversé")
  tft.fillScreen(TFT_BLACK); // Fond (ici, remplissage "en noir")

  // --------------
  // Connexion WiFi
  // --------------
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  afficher_message_erreur_sur_ecran_TFT("Recherche WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    tft.print(".");
    delay(1500);
  }

  // -----------------------------------------------------
  // Construction de l'URL cible (endpoint de l'API météo)
  // -----------------------------------------------------
  endpoint_API_meteo = "https://api.open-meteo.com/v1/forecast?latitude=" + String(LATITUDE_A_ANALYSER) + "&longitude=" + String(LONGITUDE_A_ANALYSER) +
                       "&timezone=" + TIMEZONE_CIBLEE + "&current_weather=true";
}


// =================
// Boucle principale
// =================
void loop() {

  // Vérifie si une connexion WiFi est disponible
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(endpoint_API_meteo); // Initialisation d'une connexion HTTP, avec l'endpoint choisi (entrée de l'API météo)
    int httpCode = http.GET();      // Exécute une requête HTTP GET, et récupère le code réponse

    // Vérification du code réponse HTTP reçu (404: page introuvable, 200: OK, etc)
    if (httpCode == HTTP_CODE_OK) {
      // Récupération du contenu de la réponse texte
      String contenu_reponse_HTTP = http.getString(); // Charge le contenu de la réponse HTTP, au format "String"
      DynamicJsonDocument jsonDocument(2048);         // Crée un "document JSON", qui recevra ensuite ce contenu String, au format JSON

      // Désérialise le String récupéré en HTTP/GET, pour le mettre dans ce document JSON
      DeserializationError erreur_json = deserializeJson(jsonDocument, contenu_reponse_HTTP);

      // Traitement du JSON ainsi récupéré
      if (!erreur_json) {
        // Accès aux champs qui nous intéressent
        String meteo_time = jsonDocument["current_weather"]["time"];        // Exemple de format : "2026-02-10T09:30"
        float temperature = jsonDocument["current_weather"]["temperature"]; // Exemple de format : 13.4
        float windspeed = jsonDocument["current_weather"]["windspeed"];     // Exemple de format : 10.5
        int weatherCode = jsonDocument["current_weather"]["weathercode"];   // Exemple de format : 61

        // Extraction des heures et minutes
        int tIndex = meteo_time.indexOf('T');
        String heures_et_minutes = meteo_time.substring(tIndex + 1, tIndex + 6); // Par exemple "09:30", basé sur l'exemple de datetime ci-dessus

        // Extraction du "code météo" (le code indiquant le temps qu'il fait actuellement, donc)
        if (weatherCode == 0) {
          temps_actuel = TEMPS_ENSOLEILLE;
          ligne_1_decrivant_le_temps_actuel = "Temps";
          ligne_2_decrivant_le_temps_actuel = "ensoleille !";
        } else if (weatherCode >= 1 && weatherCode <= 2) {
          temps_actuel = TEMPS_NUAGEUX;
          ligne_1_decrivant_le_temps_actuel = "Temps";
          ligne_2_decrivant_le_temps_actuel = "nuageux !";
        } else if (weatherCode == 3) {
          temps_actuel = TEMPS_COUVERT;
          ligne_1_decrivant_le_temps_actuel = "Temps";
          ligne_2_decrivant_le_temps_actuel = "couvert...";
        } else if (weatherCode == 45 || weatherCode == 48) {
          temps_actuel = TEMPS_BROUILLARD;
          ligne_1_decrivant_le_temps_actuel = "Attention";
          ligne_2_decrivant_le_temps_actuel = "brouillard !";
        } else if (weatherCode >= 51 && weatherCode <= 67) {
          temps_actuel = TEMPS_PLUVIEUX;
          ligne_1_decrivant_le_temps_actuel = "Temps";
          ligne_2_decrivant_le_temps_actuel = "pluvieux...";
        } else if (weatherCode >= 71 && weatherCode <= 77) {
          temps_actuel = TEMPS_NEIGEUX;
          ligne_1_decrivant_le_temps_actuel = "Attention :";
          ligne_2_decrivant_le_temps_actuel = "neige !!!";
        } else if (weatherCode >= 80 && weatherCode <= 82) {
          temps_actuel = TEMPS_PLUVIEUX;
          ligne_1_decrivant_le_temps_actuel = "Averses";
          ligne_2_decrivant_le_temps_actuel = "pluvieuses";
        } else if (weatherCode >= 85 && weatherCode <= 86) {
          temps_actuel = TEMPS_NEIGEUX;
          ligne_1_decrivant_le_temps_actuel = "Averses";
          ligne_2_decrivant_le_temps_actuel = "neigeuses";
        } else if (weatherCode >= 95 && weatherCode <= 99) {
          temps_actuel = TEMPS_ORAGEUX;
          ligne_1_decrivant_le_temps_actuel = "Attention";
          ligne_2_decrivant_le_temps_actuel = "ORAGE !";
        } else {
          temps_actuel = TEMPS_INCONNU;
          ligne_1_decrivant_le_temps_actuel = "TEMPS ?";
          ligne_2_decrivant_le_temps_actuel = "INCONNU";
        }

        // Affichages sur l'écran TFT
        afficher_image_de_fond_meteo_et_definir_jeu_de_couleurs();
        afficher_valeur_temperature(temperature);
        afficher_descriptif_temps_actuel();
        afficher_vitesse_vent_et_heure_de_maj(windspeed, heures_et_minutes);

        // Rebouclage (mise à jour toutes les XXX millisecondes ; par défaut 600'000 ms, soit 600 secondes, donc toutes les 10 minutes)
        delay(DELAI_RAFRAICHISSEMENT_INFOS_METEO_EN_MS);
      } else {
        // Erreur lors de désérialisation du code JSON, à partir de la réponse texte (String HTTP/GET)
        afficher_message_erreur_sur_ecran_TFT("Erreur recuperation JSON");
        delay(10000); // On relance 10 secondes après, en cas d'erreur
      }
    } else {
      // Erreur HTTP (si code retourné différent de '200')
      afficher_message_erreur_sur_ecran_TFT("Erreur HTTP: " + String(httpCode));
      delay(10000); // On relance 10 secondes après, en cas d'erreur
    }

    // Termine la connexion HTTP
    http.end();
  } else {
    // ERREUR : WiFi non trouvé ?!
    afficher_message_erreur_sur_ecran_TFT("WiFi absent ?!");
    delay(600000); // On relance 600 secondes après, soit 10 minutes après, en cas d'erreur
  }
}


// ==================================================================
// Fonction : afficher_image_de_fond_meteo_et_definir_jeu_de_couleurs
// ==================================================================
void afficher_image_de_fond_meteo_et_definir_jeu_de_couleurs() {

  if (temps_actuel == TEMPS_ENSOLEILLE) {
    tft.pushImage(0, 0, 320, 240, fond_ensoleille);                    // posX, posY, largeur, hauteur, lien_image
    couleur_texte_temperature_sur_16bits = tft.color24to16(0xF0E020);  // Couleur de la température à afficher
    couleur_texte_temps_actuel_sur_16bits = tft.color24to16(0xF0E050); // Couleur du texte descriptif sous la température actuelle
    couleur_ligne_separatrice_sur_16bits = tft.color24to16(0xB0B0C0);  // Couleur de la ligne séparatrice horizontale
    couleur_texte_du_bas_sur_16bits = tft.color24to16(0xA0A0B0);       // Couleur du texte affiché en bas (vitesse du vent, et heure de mise à jour)
  }

  if (temps_actuel == TEMPS_NUAGEUX) {
    tft.pushImage(0, 0, 320, 240, fond_nuageux);
    couleur_texte_temperature_sur_16bits = tft.color24to16(0xF0E0A0);
    couleur_texte_temps_actuel_sur_16bits = tft.color24to16(0xF0E0C0);
    couleur_ligne_separatrice_sur_16bits = tft.color24to16(0xA0A0B0);
    couleur_texte_du_bas_sur_16bits = tft.color24to16(0xA0A0B0);
  }

  if (temps_actuel == TEMPS_COUVERT) {
    tft.pushImage(0, 0, 320, 240, fond_couvert);
    couleur_texte_temperature_sur_16bits = tft.color24to16(0x6040A0);
    couleur_texte_temps_actuel_sur_16bits = tft.color24to16(0x9040A0);
    couleur_ligne_separatrice_sur_16bits = tft.color24to16(0x9040A0);
    couleur_texte_du_bas_sur_16bits = tft.color24to16(0x9040A0);
  }

  if (temps_actuel == TEMPS_BROUILLARD) {
    tft.pushImage(0, 0, 320, 240, fond_brouillard);
    couleur_texte_temperature_sur_16bits = tft.color24to16(0x808080);
    couleur_texte_temps_actuel_sur_16bits = tft.color24to16(0x909090);
    couleur_ligne_separatrice_sur_16bits = tft.color24to16(0x909090);
    couleur_texte_du_bas_sur_16bits = tft.color24to16(0x909090);
  }

  if (temps_actuel == TEMPS_PLUVIEUX) {
    tft.pushImage(0, 0, 320, 240, fond_pluvieux);
    couleur_texte_temperature_sur_16bits = tft.color24to16(0x304060);
    couleur_texte_temps_actuel_sur_16bits = tft.color24to16(0x304060);
    couleur_ligne_separatrice_sur_16bits = tft.color24to16(0x203050);
    couleur_texte_du_bas_sur_16bits = tft.color24to16(0x152545);
  }

  if (temps_actuel == TEMPS_NEIGEUX) {
    tft.pushImage(0, 0, 320, 240, fond_neigeux);
    couleur_texte_temperature_sur_16bits = tft.color24to16(0xFFFFFF);
    couleur_texte_temps_actuel_sur_16bits = tft.color24to16(0xFFFFFF);
    couleur_ligne_separatrice_sur_16bits = tft.color24to16(0xD0D0D0);
    couleur_texte_du_bas_sur_16bits = tft.color24to16(0xD0D0D0);
  }

  if (temps_actuel == TEMPS_ORAGEUX) {
    tft.pushImage(0, 0, 320, 240, fond_orageux);
    couleur_texte_temperature_sur_16bits = tft.color24to16(0xFF4000);
    couleur_texte_temps_actuel_sur_16bits = tft.color24to16(0xFF3000);
    couleur_ligne_separatrice_sur_16bits = tft.color24to16(0x808090);
    couleur_texte_du_bas_sur_16bits = tft.color24to16(0x808090);
  }
}

// ======================================
// Fonction : afficher_valeur_temperature
// ======================================
void afficher_valeur_temperature(float temperature) {

  // Formatage/affichage du texte
  tft.setTextColor(couleur_texte_temperature_sur_16bits); // Couleur du texte (codée sur 16 bits, pour être au format RGB565)
  tft.setCursor(170, 50);                                 // posX, posY du texte

  tft.setTextSize(5);        // Taille du texte
  tft.print(temperature, 1); // Texte à afficher (la valeur 'float' passée en argument de cette fonction, avec seulement 1 chiffre après la virgule)

  tft.setTextSize(3); // Taille du texte
  tft.print("÷");     // Symbole "degré" (nota : les caractères ne correspondent pas ici, donc il faut s'adapter !)
}

// ===========================================
// Fonction : afficher_descriptif_temps_actuel
// ===========================================
void afficher_descriptif_temps_actuel() {

  // Formatage/affichage du texte
  tft.setTextColor(couleur_texte_temps_actuel_sur_16bits);     // Couleur du texte (codée sur 16 bits, pour être au format RGB565)
  tft.setTextSize(2);                                          // Taille du texte
  tft.drawString(ligne_1_decrivant_le_temps_actuel, 170, 110); // Texte à afficher + posX + posY
  tft.drawString(ligne_2_decrivant_le_temps_actuel, 170, 135); // Idem
}


// ================================================
// Fonction : afficher_vitesse_vent_et_heure_de_maj
// ================================================
void afficher_vitesse_vent_et_heure_de_maj(float vitesse_du_vent, String heure_de_maj) {

  // Traçage d'une ligne horizontale, en dessous de l'image et de la valeur+description température
  tft.drawLine(5, 205, 310, 205, couleur_ligne_separatrice_sur_16bits);

  // Formatage/affichage du texte
  tft.setTextSize(2);                                // Taille du texte
  tft.setTextColor(couleur_texte_du_bas_sur_16bits); // Couleur texte
  tft.setCursor(10, 215);                            // posX, posY du texte
  tft.print("Vent:");                                // Texte à afficher
  tft.print(vitesse_du_vent, 0);                     // Le '0' indique qu'on ne souhaite pas afficher le moindre chiffre après la virgule, dans ce cas
  tft.print(" km/h - MAJ:");
  tft.print(heure_de_maj);
}

// ================================================
// Fonction : afficher_message_erreur_sur_ecran_TFT
// ================================================
void afficher_message_erreur_sur_ecran_TFT(String message_a_afficher) {

  // Affichage d'un message d'erreur en rouge, en bas de l'écran TFT
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 215);
  tft.setTextColor(tft.color24to16(0xFF0000));
  tft.print(message_a_afficher);
}
