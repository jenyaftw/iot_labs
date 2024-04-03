import * as React from 'react';
import { useEffect, useState } from 'react';
import mqttService from './mqtt';
import Map, { Marker } from 'react-map-gl/maplibre';
import 'maplibre-gl/dist/maplibre-gl.css';
import Thermometer from 'react-thermometer-component';
import './App.css';
import ReactSpeedometer from 'react-d3-speedometer';

function App() {
  const [long, setLong] = useState();
  const [lat, setLat] = useState();
  const [temp, setTemp] = useState(0);
  const [aqi, setAqi] = useState(0);

  useEffect(() => {
    const client = mqttService.getClient((err) => console.log(err));
    mqttService.onMessage(client, (mqttMessage) => {
      const { gps, sensors } = mqttMessage;
      const { temperature, aqi } = sensors;
      console.log(aqi);
      setAqi((aqi * 1000) / 4000);
      setTemp((Math.round(temperature * 100) / 100).toFixed(2));
      setLong(gps.longitude);
      setLat(gps.latitude);
    });
    mqttService.subscribe(client, 'processed_data_topic', () => {});
    return () => mqttService.closeConnection(client);
  }, []);

  return (
    <>
      <div className="map">
        <Map
          initialViewState={{
            longitude: 30.5245,
            latitude: 50.4504,
            zoom: 12,
            padding: 0,
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
      </div>
      <div className="air">
        <ReactSpeedometer
          width={400}
          needleHeightRatio={0.7}
          value={aqi}
          maxValue={1000}
          currentValueText="Air Quality Level"
          segmentColors={[
            'rgb(106, 215, 45)',
            'rgb(174, 226, 40)',
            'rgb(236, 219, 35)',
            'rgb(246, 150, 30)',
            'rgb(255, 71, 26)',
          ]}
          customSegmentLabels={[
            {
              text: 'Very Good',
              position: 'INSIDE',
              color: '#555',
            },
            {
              text: 'Good',
              position: 'INSIDE',
              color: '#555',
            },
            {
              text: 'Ok',
              position: 'INSIDE',
              color: '#555',
              fontSize: '19px',
            },

            {
              text: 'Bad',
              position: 'INSIDE',
              color: '#555',
            },
            {
              text: 'Very Bad',
              position: 'INSIDE',
              color: '#555',
            },
          ]}
          ringWidth={47}
          needleTransitionDuration={3333}
          needleTransition="easeElastic"
          needleColor={'#90f2ff'}
          textColor={'black'}
        />
      </div>
      <div className="thermometer">
        <Thermometer
          theme="light"
          value={temp}
          max="100"
          steps="3"
          format="°C"
          size="medium"
          height="200"
        />
      </div>
    </>
  );
}

export default App;
