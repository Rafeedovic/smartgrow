import { Component, OnInit } from '@angular/core';
import { SensorDataService } from './services/sensor-data.service';
import { AngularFireDatabase } from '@angular/fire/compat/database';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  sensorData: any = {}; // Initialisation vide
  title = 'smartgrow';  // Définir la propriété title
  lampStatus: boolean = false;
  ventiloStatus: boolean = false;
  irrigationStatus: boolean = false;
  porteStatus: boolean = false;

  constructor(
    private sensorDataService: SensorDataService,
    private db: AngularFireDatabase  // Make sure this line is correctly added
  ) {}

  ngOnInit(): void {
    this.sensorDataService.getSensorData().subscribe(data => {
      console.log(data); // Vérifiez si les données sont récupérées dans la console
      this.sensorData = data; // Affecte les données récupérées
    }, error => {
      console.error("Erreur lors de la récupération des données : ", error);
    }); // Ferme le subscribe correctement
  } // Ferme le ngOnInit correctement

  toggleLamps(): void {
    const currentTime = new Date().toISOString(); // ISO string for the timestamp
    const updates = {
      '/lampes_allumees': this.lampStatus,
      '/allumage_lampes_time': currentTime // Update the timestamp
    };
  
    this.db.object('/').update(updates) // Updating at the root of the database path
      .then(() => {
        console.log(`Lampes ${this.lampStatus ? 'allumées' : 'éteintes'} à ${currentTime}!`);
      })
      .catch(err => {
        console.error('Erreur lors de l\'allumage/éteignage des lampes:', err);
      });
  }

  toggleVentilo(): void {
    const currentTime = new Date().toISOString(); // ISO string for the timestamp
    const updates = {
      '/ventilateur_allume': this.ventiloStatus,
      '/ventilateur_allume_time': currentTime // Update the timestamp
    };
  
    this.db.object('/').update(updates) // Updating at the root of the database path
      .then(() => {
        console.log(`Ventilos ${this.ventiloStatus ? 'actionnés' : 'éteintes'} à ${currentTime}!`);
      })
      .catch(err => {
        console.error('Erreur lors de l\'actionnement/éteignage des ventilos:', err);
      });
  }

  toggleIrrigation(): void {
    const currentTime = new Date().toISOString(); // ISO string for the timestamp
    const updates = {
      '/irrigation_active': this.irrigationStatus,
      '/irrigation_active_time': currentTime // Update the timestamp
    };
  
    this.db.object('/').update(updates) // Updating at the root of the database path
      .then(() => {
        console.log(`Irrigation ${this.irrigationStatus ? 'actionnés' : 'éteintes'} à ${currentTime}!`);
      })
      .catch(err => {
        console.error('Erreur lors de l\'irrigation/éteignage:', err);
      });
  }

  togglePorte(): void {
    const currentTime = new Date().toISOString(); // ISO string for the timestamp
    const updates = {
      '/porte_ouverte': this.porteStatus,
      '/porte_ouverte_time': currentTime // Update the timestamp
    };
  
    this.db.object('/').update(updates) // Updating at the root of the database path
      .then(() => {
        console.log(`Porte ${this.porteStatus ? 'ouverte' : 'fermés'} à ${currentTime}!`);
      })
      .catch(err => {
        console.error('Erreur lors de l\'ouverture/fermeture:', err);
      });
  }
  
}