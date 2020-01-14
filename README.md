# Vesta


- Mesure de températures avec sondes DS18B20
- Supporte des mesure déportées sur des modules alimentés par batterie
- Régule la température sur 6 canaux via une interface I2C
- Chaque canal permet 8 points de contrôle par jour de la semaine
- Les mesures sont enregistrées toutes les 15 minutes sur une carte SD
- L'horloge du module est synchronisée avec un serveur NTP
- Le module comporte un serveur web
- Le serveur est accessible via internet et localement par mDNS (Bonjour) 
- Le serveur permet de:
	- Modifier les points de controle
	- Visualiser l'état du modules et de la régulation
	- Visualiser les courbes de températures pour chaque journée
	- Ajouter ou modifier des capteurs de température
	- Vérifier l'état de la batterie des capteurs déportés
	- Voir les fichiers de configuration
- Le module peut envoyer des courriels en cas de problème
