import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable, of } from 'rxjs';


@Injectable({
  providedIn: 'root'
})
export class LocationService {
  locationUrl: string = 'http://localhost:5000/location'

  constructor(private http: HttpClient) {}

  getLocation(): Observable<any> {
    return this.http.get(this.locationUrl);
  }
}
