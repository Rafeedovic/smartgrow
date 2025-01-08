import { Component, OnInit } from '@angular/core';
import { SensorDataService } from './services/sensor-data.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  sensorData: any = {}; // Initialisation vide
  title = 'smartgrow';  // Définir la propriété title

  constructor(private sensorDataService: SensorDataService) {}

  ngOnInit(): void {
    this.sensorDataService.getSensorData().subscribe(data => {
      console.log(data); // Vérifiez si les données sont récupérées dans la console
      this.sensorData = data; // Affecte les données récupérées
    }, error => {
      console.error("Erreur lors de la récupération des données : ", error);
    }); // Ferme le subscribe correctement
  } // Ferme le ngOnInit correctement
}