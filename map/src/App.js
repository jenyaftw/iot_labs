import * as React from 'react';
import { useEffect, useState } from 'react';
import mqttService from './mqtt';
import Map, { Marker } from 'react-map-gl/maplibre';
import 'maplibre-gl/dist/maplibre-gl.css';

function App() {
  const [long, setLong] = useState();
  const [lat, setLat] = useState();
  useEffect(() => {
    const client = mqttService.getClient((err) => console.log(err));
    mqttService.onMessage(client, (mqttMessage) => {
      const { gps } = mqttMessage;
      console.log(mqttMessage);
      setLong(gps.longitude);
      setLat(gps.latitude);
    });
    mqttService.subscribe(client, 'processed_data_topic', () => {});
    return () => mqttService.closeConnection(client);
  }, []);

  return (
    <Map
      initialViewState={{
        longitude: 30.5245,
        latitude: 50.4504,
        zoom: 12,
      }}
      style={{ width: '100vw', height: '100vh' }}
      mapStyle="https://api.maptiler.com/maps/streets/style.json?key=DZ3M3QFwgoHbaT8TjMkf"
    >
      <Marker
        longitude={long ? long : 30.5245}
        latitude={lat ? lat : 50.4504}
        anchor="bottom"
      >
        <img src="/car.png" />
      </Marker>
    </Map>
  );
}

export default App;
