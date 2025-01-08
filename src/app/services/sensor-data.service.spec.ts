import { Injectable } from '@angular/core';
import { AngularFireDatabase } from '@angular/fire/compat/database';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class SensorDataService {

  constructor(private db: AngularFireDatabase) {}

  // Fonction pour récupérer toutes les données depuis la racine
  getSensorData(): Observable<any> {
    return this.db.object('/').valueChanges(); // Accéder à la racine
  }
}