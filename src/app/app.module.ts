import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { RouterModule } from '@angular/router';
import { AngularFireModule } from '@angular/fire/compat'; 
import { AngularFireDatabaseModule } from '@angular/fire/compat/database'; 
import { environment } from '../environments/environment';
import { AppComponent } from './app.component';

@NgModule({
  declarations: [AppComponent],
  imports: [
    BrowserModule,
    RouterModule, 
    AngularFireModule.initializeApp(environment.firebaseConfig), 
    AngularFireDatabaseModule, 
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule {}
