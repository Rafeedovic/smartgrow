import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { RouterModule } from '@angular/router';
import { AngularFireModule } from '@angular/fire/compat'; 
import { AngularFireDatabaseModule } from '@angular/fire/compat/database'; 
import { environment } from '../environments/environment';
import { AppComponent } from './app.component';
import { FormsModule } from '@angular/forms';


@NgModule({
  declarations: [AppComponent],
  imports: [
    BrowserModule,
    RouterModule, 
    FormsModule,
    AngularFireModule.initializeApp(environment.firebaseConfig), 
    AngularFireDatabaseModule, 
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule {}
