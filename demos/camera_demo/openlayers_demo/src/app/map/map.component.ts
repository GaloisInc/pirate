import { AfterViewInit, Component, OnInit } from '@angular/core';
import { Observable, of } from 'rxjs';
import View from 'ol/View';
import Attribution from 'ol/control/Attribution';
import FullScreen from 'ol/control/FullScreen';
import ScaleLine from 'ol/control/ScaleLine';
import Feature from 'ol/Feature';
import Point from 'ol/geom/Point';
import LayerTile from 'ol/layer/Tile';
import SourceOsm from 'ol/source/OSM';
import { defaults as defaultControls } from 'ol/control';
import { defaults as defaultInteractions, PinchZoom } from 'ol/interaction';

import { LocationService } from '../location.service';
import { GeoMap } from '@model/map';

@Component({
  selector: 'app-map',
  templateUrl: './map.component.html',
  styleUrls: ['./map.component.css']
})

export class MapComponent implements OnInit, AfterViewInit {
  map: GeoMap;
  location: [number, number];
  feature: Feature;

  constructor(private locationService: LocationService) {}

  ngOnInit() {
    setInterval(() => this.getLocation(), 1000);
  }

  ngAfterViewInit() {
    this.map = new GeoMap();
    this.setLocationMarker();
  }

  setLocationMarker() {
    if (!this.location) {
      if (this.feature) {
        this.map.source.removeFeature(this.feature);
        this.feature = null;
      }
      return;
    }
    if (!this.feature) {
      this.feature = new Feature({
        geometry: new Point(this.location)
      });
      this.map.source.addFeature(this.feature);
    } else {
      (this.feature.getGeometry() as Point).setCoordinates(this.location);
    }
  }

  getLocation(): void {
    this.locationService.getLocation().subscribe(location => {
      this.location = [location.longitude, location.latitude];
      this.setLocationMarker();
    });
  }
}
