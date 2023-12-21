# EDF-Tempo
Boitier LED pour indiquer la couleur du jour de l'abonnement Tempo d'EDF à partir d'une carte ESP32

## branchement des LED:
![Cover](https://github.com/Bosoochee/EDF-Tempo/blob/main/docs/images/esp32-devkitC-v4-pinout.png)

### Couleur du jour:
   - Bleu :  pin GIOP12
   - Blanc : pin GIOP14
   - Rouge : pin GIOP27

### Couleur du lendemain :
   - Bleu :  pin GIOP16
   - Blanc : pin GIOP17
   - Rouge : pin GIOP18

### Led d'indication des heures creuses/pleines
   - Heure creuse : pin GIOP33
   - Heure pleine : pin GIOP33

### Installation
   - Indiquer le SSID et le mot de passe de votre WIFI local dans le code ligne 29et 30 (variables SSID_NAME et SSID_PASS)
