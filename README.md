# Projet-de-PFE
# Système de Réglage de Température IoT avec Interface Web
L'objectif est de permettre à l'utilisateur de contrôler et de régler la température de son environnement à distance.

## Technologies Utilisées

- **Matériel** : carte ESP8266 + le capteur DS18B20, qui est connecté au thermostat via une connexion Wi-Fi

- **Environnement de Programmation** : L'IDE Arduino 

- **Environnement de Développement Web** : Pour l'interface utilisateur, HTML + CSS +JS

### Fonctionnalités Principales

1. **Interface d'Authentification**
    - L'interface d'authentification permet aux utilisateurs d'accéder au système en saisissant leur nom d'utilisateur et leur mot de passe.
    - En cas d'échec, un message d'erreur s'affiche, invitant l'utilisateur à réessayer avec des informations correctes.

2. **Page d'Accueil**
    - La page d'accueil s'affiche après une connexion réussie et comporte trois boutons : "Activer", "Désactiver", et "Automatique".
    - La température détectée par le capteur et la température consigne demandée par le client dans une instance particulière sont affichées.
    - La page affiche également la date correspondante et l'heure de l'instance actuelle.
    - Le système compare périodiquement la valeur détectée par le capteur à la valeur consigne et active le thermostat en fonction de la comparaison.

3. **Page d'Accueil : Mode Désactivé**
    - Ce mode bloque l'activation du thermostat.

4. **Page d'Accueil : Mode Activé**
    - Ce mode maintient l'activation du thermostat.

5. **Page Tableau de Bord**
    - La page de tableau de bord affiche sept boutons, chacun correspondant à un jour de la semaine avec une couleur unique.
    - Un graphe multiligne affiche les événements ajoutés par le client, montrant les valeurs de température à différents moments.
    - Les lignes du graphe sont colorées en fonction du jour correspondant.
    - Le clic sur un bouton fait basculer la ligne correspondante entre apparaître et disparaître.

6. **Page d'Ajout d'un Événement**
    - Cette page permet aux clients d'ajouter un ou plusieurs événements en remplissant un formulaire.
    - Les clients peuvent sélectionner les jours souhaités, la température consigne, et l'heure désirée.

7. **Page d'Info-Bulle**
    - En survolant le pointeur sur n'importe quel point du graphique, une info-bulle s'affiche, fournissant des informations sur cette instance particulière.
    - Les utilisateurs ont la possibilité de supprimer ou de modifier l'événement en fournissant les boutons "Supprimer" et "Modifier".

8. **Page de Modification d'un Événement**
    - En cliquant sur le bouton "Modifier" du graphe, les utilisateurs accèdent au même formulaire que celui de la page d'ajout, mais avec les informations préremplies pour le point sélectionné, ce qui facilite la modification.

9. **Page de Configuration**
    - Cette page offre la possibilité de configurer le réseau Wi-Fi auquel le système est connecté en fournissant les informations du réseau (SSID et mot de passe).
    







