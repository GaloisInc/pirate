import Map from 'ol/Map';
import View from 'ol/View';
import LayerTile from 'ol/layer/Tile';
import FullScreen from 'ol/control/FullScreen';
import ScaleLine from 'ol/control/ScaleLine';
import Attribution from 'ol/control/Attribution';
import SourceOsm from 'ol/source/OSM';
import VectorLayer from 'ol/layer/Vector';
import VectorSource from 'ol/source/Vector';
import { defaults as defaultControls } from 'ol/control';

/**
 * Geographical map containing some basic controls
 */
export class GeoMap {
  readonly map: Map;
  readonly source: VectorSource;

  constructor() {
    this.source = new VectorSource();
    this.map = new Map({
      target: 'map',
      layers: [
        new LayerTile({source: new SourceOsm()}),
        new VectorLayer({source: this.source})
      ],
      view: new View({
        projection: 'EPSG:4326',
        center: [0, 0],
        zoom: 2,
        constrainResolution: true,
        extent: [-180, -90, 180, 90]
      }),
      controls: defaultControls().extend([
        new Attribution(),
        new FullScreen(),
        new ScaleLine({
          bar: true,
          minWidth: 150,
          units: 'imperial'
        })
      ])
    });
  }
}
