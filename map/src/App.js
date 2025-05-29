import * as React from 'react';
import { useEffect, useState, useRef } from 'react';
import mqttService from './mqtt';
import Map, { Marker, Source, Layer } from 'react-map-gl/maplibre';
import 'maplibre-gl/dist/maplibre-gl.css';
import './App.css';
import ReactSpeedometer from 'react-d3-speedometer';

function App() {
  const [long, setLong] = useState();
  const [lat, setLat] = useState();
  const [temp, setTemp] = useState(0);
  const [aqi, setAqi] = useState(0);
  const [sensorStatus, setSensorStatus] = useState('offline');
  const [rssi, setRssi] = useState(0);
  const [battery, setBattery] = useState(0);
  const [gpsValid, setGpsValid] = useState(false);
  const [accelerometer, setAccelerometer] = useState({ x: 0, y: 0, z: 0, magnitude: 0 });

  // Add ref for recording state
  const isRecordingRef = useRef(false);
  const [isRecording, setIsRecording] = useState(false);

  // Trip recording states
  const [tripData, setTripData] = useState([]);
  const [tripPath, setTripPath] = useState([]);
  const [currentSpeed, setCurrentSpeed] = useState(0);
  const [tripStartTime, setTripStartTime] = useState(null);
  const [tripStats, setTripStats] = useState({
    duration: 0,
    distance: 0,
    avgSpeed: 0,
    maxSpeed: 0,
    avgAirQuality: 0,
    maxAcceleration: 0
  });

  // Trip history states
  const [savedTrips, setSavedTrips] = useState([]);
  const [showTripHistory, setShowTripHistory] = useState(false);
  const [selectedTrip, setSelectedTrip] = useState(null);
  const [viewMode, setViewMode] = useState('live'); // 'live' or 'history'

  // Helper function to calculate distance between two GPS points
  const calculateDistance = (lat1, lon1, lat2, lon2) => {
    const R = 6371; // Earth's radius in km
    const dLat = (lat2 - lat1) * Math.PI / 180;
    const dLon = (lon2 - lon1) * Math.PI / 180;
    const a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
      Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
      Math.sin(dLon / 2) * Math.sin(dLon / 2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
    return R * c; // Distance in km
  };

  // Helper function to calculate speed from accelerometer
  const calculateSpeedFromAccel = (accelData, prevSpeed, deltaTime) => {
    // Remove gravity component from Z-axis (assuming device is mostly upright)
    const gravityFiltered = {
      x: accelData.x,
      y: accelData.y,
      z: accelData.z - 9.81 // Remove standard gravity
    };

    // Calculate horizontal acceleration (ignoring vertical movement)
    const horizontalAccel = Math.sqrt(gravityFiltered.x ** 2 + gravityFiltered.y ** 2);

    // Only consider significant acceleration changes (filter noise)
    if (horizontalAccel < 0.5) {
      // Apply decay to speed when no significant acceleration
      return Math.max(0, prevSpeed * 0.95);
    }

    // Simple integration with damping: v = v0 + a*t
    const deltaTimeSeconds = deltaTime / 1000;
    const newSpeed = Math.max(0, prevSpeed + (horizontalAccel * deltaTimeSeconds));

    // Convert to km/h and apply realistic limits
    const speedKmh = newSpeed * 3.6;

    // Cap at reasonable maximum speed (e.g., 200 km/h)
    return Math.min(speedKmh, 200);
  };

  // Helper function to calculate speed from GPS
  const calculateSpeedFromGPS = (currentGPS, previousGPS, deltaTime) => {
    if (!currentGPS || !previousGPS || !currentGPS.valid || !previousGPS.valid) {
      return null;
    }

    const distance = calculateDistance(
      previousGPS.latitude, previousGPS.longitude,
      currentGPS.latitude, currentGPS.longitude
    );

    const deltaTimeHours = deltaTime / (1000 * 3600); // Convert to hours
    return deltaTimeHours > 0 ? distance / deltaTimeHours : 0;
  };

  const startTrip = () => {
    isRecordingRef.current = true;
    setIsRecording(true);
    setTripStartTime(Date.now());
    setTripData([]);
    setTripPath([]);
    setCurrentSpeed(0);
    setTripStats({
      duration: 0,
      distance: 0,
      avgSpeed: 0,
      maxSpeed: 0,
      avgAirQuality: 0,
      maxAcceleration: 0
    });
  };

  const stopTrip = () => {
    isRecordingRef.current = false;
    setIsRecording(false);
    if (tripData.length > 0) {
      // Calculate final trip statistics
      const totalDistance = tripStats.distance;
      const totalTime = (Date.now() - tripStartTime) / 1000 / 3600; // hours
      const avgSpeed = totalTime > 0 ? totalDistance / totalTime : 0;
      const avgAirQuality = tripData.reduce((sum, point) => sum + point.airQuality, 0) / tripData.length;
      const maxAcceleration = Math.max(...tripData.map(point => point.acceleration));
      const maxSpeed = Math.max(...tripData.map(point => point.speed));

      const finalStats = {
        duration: (Date.now() - tripStartTime) / 1000,
        distance: totalDistance,
        avgSpeed: avgSpeed,
        avgAirQuality: avgAirQuality,
        maxAcceleration: maxAcceleration,
        maxSpeed: maxSpeed
      };

      setTripStats(finalStats);

      // Save trip to history
      const completedTrip = {
        id: Date.now(),
        startTime: tripStartTime,
        endTime: Date.now(),
        data: [...tripData],
        path: [...tripPath],
        stats: finalStats
      };

      setSavedTrips(prev => [completedTrip, ...prev]);
    }
  };

  const downloadTripData = () => {
    if (tripData.length === 0) return;

    const csvContent = [
      'timestamp,latitude,longitude,speed_kmh,acceleration_ms2,air_quality,battery_v',
      ...tripData.map(point =>
        `${point.timestamp},${point.latitude},${point.longitude},${point.speed},${point.acceleration},${point.airQuality},${point.battery}`
      )
    ].join('\n');

    const blob = new Blob([csvContent], { type: 'text/csv' });
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `trip_data_${new Date().toISOString().split('T')[0]}.csv`;
    a.click();
    window.URL.revokeObjectURL(url);
  };

  const viewTrip = (trip) => {
    setSelectedTrip(trip);
    setViewMode('history');
    setShowTripHistory(false);
  };

  const backToLive = () => {
    setViewMode('live');
    setSelectedTrip(null);
  };

  const deleteTrip = (tripId) => {
    setSavedTrips(prev => prev.filter(trip => trip.id !== tripId));
    if (selectedTrip && selectedTrip.id === tripId) {
      backToLive();
    }
  };

  const formatDate = (timestamp) => {
    return new Date(timestamp).toLocaleString();
  };

  const formatDuration = (seconds) => {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);

    if (hours > 0) {
      return `${hours}h ${minutes}m ${secs}s`;
    } else if (minutes > 0) {
      return `${minutes}m ${secs}s`;
    } else {
      return `${secs}s`;
    }
  };

  useEffect(() => {
    const username = process.env.REACT_APP_HIVEMQ_USERNAME;
    const password = process.env.REACT_APP_HIVEMQ_PASSWORD;

    const client = mqttService.getClient(
      (err) => console.log(err),
      username,
      password
    );

    mqttService.onMessage(client, (topic, mqttMessage) => {
      console.log(`Received message on topic: ${topic}`, mqttMessage);

      if (topic === 'lora/sensor/data') {
        const { gps, air_quality, battery, accelerometer } = mqttMessage;
        const timestamp = Date.now();

        // Handle GPS data
        if (gps && gps.valid) {
          console.log('Received valid GPS data:', gps);
          setLong(gps.longitude);
          setLat(gps.latitude);
          setGpsValid(true);

          // Record path point if recording is active and GPS is valid
          if (isRecordingRef.current) {
            console.log('Recording is active (ref), adding GPS point to path:', [gps.longitude, gps.latitude]);
            setTripPath(prevPath => {
              const newPath = [...prevPath, [gps.longitude, gps.latitude]];
              console.log('Previous path length:', prevPath.length);
              console.log('New path length:', newPath.length);
              console.log('Full path:', newPath);
              return newPath;
            });

            // Update distance stats
            setTripStats(prevStats => {
              const lastPathPoint = tripPath[tripPath.length - 1];
              let newDistance = 0;

              if (lastPathPoint && lastPathPoint.length === 2) {
                newDistance = calculateDistance(
                  lastPathPoint[1], lastPathPoint[0], // lat, lon
                  gps.latitude, gps.longitude
                );
              }

              return {
                ...prevStats,
                distance: prevStats.distance + newDistance
              };
            });
          } else {
            console.log('Recording is not active (ref), skipping path update');
          }
        } else {
          console.log('Invalid or missing GPS data:', gps);
          setGpsValid(false);
        }

        // Handle air quality (scale to 0-1000 range for the speedometer)
        if (air_quality !== undefined) {
          setAqi(Math.min(air_quality, 500)); // Cap at max value to stay within scale
        }

        // Handle battery
        if (battery !== undefined) {
          setBattery(battery);
        }

        // Handle accelerometer data
        if (accelerometer) {
          const magnitude = Math.sqrt(
            accelerometer.x ** 2 +
            accelerometer.y ** 2 +
            accelerometer.z ** 2
          );
          const newAccelData = {
            x: accelerometer.x,
            y: accelerometer.y,
            z: accelerometer.z,
            magnitude: magnitude
          };
          setAccelerometer(newAccelData);
          setTemp((magnitude).toFixed(2));

          // Trip recording logic for accelerometer data
          if (isRecordingRef.current) {
            setTripData(prevData => {
              const lastPoint = prevData[prevData.length - 1];
              let newSpeed = 0;

              if (lastPoint) {
                const deltaTime = timestamp - lastPoint.timestamp;

                // Try GPS-based speed calculation first (more accurate)
                const gpsSpeed = calculateSpeedFromGPS(gps,
                  lastPoint.latitude && lastPoint.longitude ?
                    { valid: true, latitude: lastPoint.latitude, longitude: lastPoint.longitude } : null,
                  deltaTime
                );

                if (gpsSpeed !== null) {
                  newSpeed = gpsSpeed;
                } else {
                  // Fallback to accelerometer-based calculation
                  newSpeed = calculateSpeedFromAccel(newAccelData, lastPoint.speed, deltaTime);
                }
              }

              setCurrentSpeed(newSpeed);

              const newPoint = {
                timestamp: timestamp,
                latitude: gps && gps.valid ? gps.latitude : null,
                longitude: gps && gps.valid ? gps.longitude : null,
                speed: newSpeed,
                acceleration: magnitude,
                airQuality: air_quality || 0,
                battery: battery || 0
              };

              console.log('Recording trip point:', newPoint);
              return [...prevData, newPoint];
            });
          }
        }
      } else if (topic === 'lora/sensor/status') {
        setSensorStatus(mqttMessage);
      } else if (topic === 'lora/sensor/rssi') {
        setRssi(mqttMessage);
      }
    });

    // Subscribe to all LoRa sensor topics
    mqttService.subscribe(client, 'lora/sensor/data', (err) => {
      if (err) console.log('Failed to subscribe to lora/sensor/data');
    });
    mqttService.subscribe(client, 'lora/sensor/status', (err) => {
      if (err) console.log('Failed to subscribe to lora/sensor/status');
    });
    mqttService.subscribe(client, 'lora/sensor/rssi', (err) => {
      if (err) console.log('Failed to subscribe to lora/sensor/rssi');
    });

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
          {/* Trip path visualization */}
          {(() => {
            console.log('Rendering map - viewMode:', viewMode, 'tripPath.length:', tripPath.length, 'isRecording:', isRecording);
            console.log('Should show path:', viewMode === 'live' && tripPath.length > 1);
            return null;
          })()}
          {viewMode === 'live' && tripPath.length > 1 && (
            <Source
              id="trip-path"
              type="geojson"
              data={{
                type: 'Feature',
                properties: {},
                geometry: {
                  type: 'LineString',
                  coordinates: tripPath
                }
              }}
            >
              <Layer
                id="trip-path-layer"
                type="line"
                paint={{
                  'line-color': '#ff6b35',
                  'line-width': 6,
                  'line-opacity': 1.0
                }}
                layout={{
                  'line-join': 'round',
                  'line-cap': 'round'
                }}
              />
            </Source>
          )}

          {/* Fallback: Show path points as individual markers if line doesn't work */}
          {viewMode === 'live' && tripPath.length > 1 && tripPath.map((coord, index) => (
            index > 0 && (
              <Marker
                key={`path-point-${index}`}
                longitude={coord[0]}
                latitude={coord[1]}
              >
                <div style={{
                  width: '8px',
                  height: '8px',
                  backgroundColor: '#ff6b35',
                  borderRadius: '50%',
                  border: '2px solid white'
                }} />
              </Marker>
            )
          ))}

          {/* Historical trip path */}
          {viewMode === 'history' && selectedTrip && selectedTrip.path.length > 1 && (
            <Source
              id="history-trip-path"
              type="geojson"
              data={{
                type: 'Feature',
                properties: {},
                geometry: {
                  type: 'LineString',
                  coordinates: selectedTrip.path
                }
              }}
            >
              <Layer
                id="history-trip-path-layer"
                type="line"
                paint={{
                  'line-color': '#2196F3',
                  'line-width': 4,
                  'line-opacity': 0.8
                }}
              />
            </Source>
          )}

          <Marker
            longitude={viewMode === 'live' ? (long && gpsValid ? long : 30.5245) :
              (selectedTrip && selectedTrip.path.length > 0 ? selectedTrip.path[selectedTrip.path.length - 1][0] : 30.5245)}
            latitude={viewMode === 'live' ? (lat && gpsValid ? lat : 50.4504) :
              (selectedTrip && selectedTrip.path.length > 0 ? selectedTrip.path[selectedTrip.path.length - 1][1] : 50.4504)}
            anchor="bottom"
          >
            <img src="/car.png" alt="sensor location" />
          </Marker>
        </Map>
      </div>

      {/* Status indicator */}
      <div className="status">
        <div className={`status-indicator ${sensorStatus}`}>
          <span>Sensor: {sensorStatus}</span>
          {gpsValid ? (
            <span>GPS: Valid</span>
          ) : (
            <span>GPS: Invalid</span>
          )}
          <span>RSSI: {rssi} dBm</span>
          <span>Battery: {battery}V</span>
        </div>
      </div>

      {/* Accelerometer data display */}
      <div className="accelerometer">
        <div className="accelerometer-panel">
          <h3>Accelerometer</h3>
          <div className="accel-data">
            <span>X: {accelerometer.x.toFixed(3)} m/s²</span>
            <span>Y: {accelerometer.y.toFixed(3)} m/s²</span>
            <span>Z: {accelerometer.z.toFixed(3)} m/s²</span>
            <span className="magnitude">Magnitude: {accelerometer.magnitude.toFixed(3)} m/s²</span>
          </div>
        </div>
      </div>

      <div className="accelerometer-bar">
        <div className="bar-container">
          <div className="bar-label">Acceleration Magnitude</div>
          <div className="bar-wrapper">
            <div
              className="bar-fill"
              style={{
                width: `${Math.min((accelerometer.magnitude / 15) * 100, 100)}%`,
                backgroundColor: accelerometer.magnitude > 12 ? '#ff4444' :
                  accelerometer.magnitude > 10 ? '#ffaa00' : '#44ff44'
              }}
            ></div>
            <div className="bar-value">{accelerometer.magnitude.toFixed(2)} m/s²</div>
          </div>
          <div className="bar-scale">
            <span>0</span>
            <span>7.5</span>
            <span>15 m/s²</span>
          </div>
        </div>
      </div>

      <div className="air">
        <ReactSpeedometer
          width={400}
          needleHeightRatio={0.7}
          value={aqi}
          maxValue={500}
          minValue={0}
          currentValueText="Air Quality Index"
          segmentColors={[
            'rgb(106, 215, 45)',  // 0-50: Good (Green)
            'rgb(174, 226, 40)',  // 50-100: Moderate (Light Green)
            'rgb(236, 219, 35)',  // 100-150: Unhealthy for Sensitive (Yellow)
            'rgb(246, 150, 30)',  // 150-300: Unhealthy (Orange)
            'rgb(255, 71, 26)',   // 300-500: Very Unhealthy (Red)
          ]}
          customSegmentLabels={[
            {
              text: 'Good',
              position: 'INSIDE',
              color: '#555',
              fontSize: '14px',
            },
            {
              text: 'Moderate',
              position: 'INSIDE',
              color: '#555',
              fontSize: '12px',
            },
            {
              text: 'Sensitive',
              position: 'INSIDE',
              color: '#555',
              fontSize: '11px',
            },
            {
              text: 'Unhealthy',
              position: 'INSIDE',
              color: '#555',
              fontSize: '12px',
            },
            {
              text: 'Very Bad',
              position: 'INSIDE',
              color: '#555',
              fontSize: '12px',
            },
          ]}
          customSegmentStops={[0, 50, 100, 150, 300, 500]}
          ringWidth={47}
          needleTransitionDuration={3333}
          needleTransition="easeElastic"
          needleColor={'#90f2ff'}
          textColor={'black'}
        />
      </div>

      {/* Trip Recording Controls */}
      <div className="trip-controls">
        <div className="trip-panel">
          <h3>{viewMode === 'live' ? 'Trip Recording' : 'Trip History'}</h3>

          {viewMode === 'live' ? (
            <>
              <div className="trip-buttons">
                {!isRecording ? (
                  <button className="start-trip-btn" onClick={startTrip}>
                    Start Trip
                  </button>
                ) : (
                  <button className="stop-trip-btn" onClick={stopTrip}>
                    Stop Trip
                  </button>
                )}
                {tripData.length > 0 && (
                  <button className="download-btn" onClick={downloadTripData}>
                    Download Data
                  </button>
                )}
                {savedTrips.length > 0 && (
                  <button className="history-btn" onClick={() => setShowTripHistory(true)}>
                    View History ({savedTrips.length})
                  </button>
                )}
              </div>

              {isRecording && (
                <div className="recording-indicator">
                  <span className="recording-dot"></span>
                  Recording...
                </div>
              )}

              <div className="trip-stats">
                <div className="stat-row">
                  <span>Current Speed:</span>
                  <span>{currentSpeed.toFixed(1)} km/h</span>
                </div>
                <div className="stat-row">
                  <span>Duration:</span>
                  <span>{isRecording && tripStartTime ? Math.floor((Date.now() - tripStartTime) / 1000) : Math.floor(tripStats.duration)} sec</span>
                </div>
                <div className="stat-row">
                  <span>Distance:</span>
                  <span>{tripStats.distance.toFixed(2)} km</span>
                </div>
                <div className="stat-row">
                  <span>Path Points:</span>
                  <span>{tripPath.length}</span>
                </div>
                <div className="stat-row">
                  <span>GPS Valid:</span>
                  <span>{gpsValid ? 'Yes' : 'No'}</span>
                </div>
                <div className="stat-row">
                  <span>View Mode:</span>
                  <span>{viewMode}</span>
                </div>
                {tripData.length > 0 && (
                  <>
                    <div className="stat-row">
                      <span>Avg Speed:</span>
                      <span>{tripStats.avgSpeed.toFixed(1)} km/h</span>
                    </div>
                    <div className="stat-row">
                      <span>Max Speed:</span>
                      <span>{tripStats.maxSpeed.toFixed(1)} km/h</span>
                    </div>
                    <div className="stat-row">
                      <span>Avg Air Quality:</span>
                      <span>{tripStats.avgAirQuality.toFixed(0)}</span>
                    </div>
                    <div className="stat-row">
                      <span>Data Points:</span>
                      <span>{tripData.length}</span>
                    </div>
                  </>
                )}
              </div>
            </>
          ) : (
            <>
              <div className="trip-buttons">
                <button className="back-btn" onClick={backToLive}>
                  Back to Live
                </button>
                <button className="download-btn" onClick={() => {
                  if (selectedTrip) {
                    const csvContent = [
                      'timestamp,latitude,longitude,speed_kmh,acceleration_ms2,air_quality,battery_v',
                      ...selectedTrip.data.map(point =>
                        `${point.timestamp},${point.latitude},${point.longitude},${point.speed},${point.acceleration},${point.airQuality},${point.battery}`
                      )
                    ].join('\n');
                    const blob = new Blob([csvContent], { type: 'text/csv' });
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = `trip_${selectedTrip.id}.csv`;
                    a.click();
                    window.URL.revokeObjectURL(url);
                  }
                }}>
                  Download
                </button>
                <button className="delete-btn" onClick={() => {
                  if (selectedTrip && window.confirm('Delete this trip?')) {
                    deleteTrip(selectedTrip.id);
                  }
                }}>
                  Delete
                </button>
              </div>

              {selectedTrip && (
                <div className="trip-stats">
                  <div className="stat-row">
                    <span>Date:</span>
                    <span>{formatDate(selectedTrip.startTime)}</span>
                  </div>
                  <div className="stat-row">
                    <span>Duration:</span>
                    <span>{formatDuration(selectedTrip.stats.duration)}</span>
                  </div>
                  <div className="stat-row">
                    <span>Distance:</span>
                    <span>{selectedTrip.stats.distance.toFixed(2)} km</span>
                  </div>
                  <div className="stat-row">
                    <span>Avg Speed:</span>
                    <span>{selectedTrip.stats.avgSpeed.toFixed(1)} km/h</span>
                  </div>
                  <div className="stat-row">
                    <span>Max Speed:</span>
                    <span>{selectedTrip.stats.maxSpeed.toFixed(1)} km/h</span>
                  </div>
                  <div className="stat-row">
                    <span>Avg Air Quality:</span>
                    <span>{selectedTrip.stats.avgAirQuality.toFixed(0)}</span>
                  </div>
                  <div className="stat-row">
                    <span>Max Acceleration:</span>
                    <span>{selectedTrip.stats.maxAcceleration.toFixed(2)} m/s²</span>
                  </div>
                  <div className="stat-row">
                    <span>Data Points:</span>
                    <span>{selectedTrip.data.length}</span>
                  </div>
                </div>
              )}
            </>
          )}
        </div>
      </div>

      {/* Trip History Modal */}
      {showTripHistory && (
        <div className="trip-history-overlay">
          <div className="trip-history-modal">
            <div className="modal-header">
              <h2>Trip History</h2>
              <button className="close-btn" onClick={() => setShowTripHistory(false)}>×</button>
            </div>
            <div className="trip-list">
              {savedTrips.length === 0 ? (
                <p className="no-trips">No trips recorded yet</p>
              ) : (
                savedTrips.map(trip => (
                  <div key={trip.id} className="trip-item">
                    <div className="trip-info">
                      <div className="trip-date">{formatDate(trip.startTime)}</div>
                      <div className="trip-summary">
                        <span>{formatDuration(trip.stats.duration)}</span>
                        <span>{trip.stats.distance.toFixed(2)} km</span>
                        <span>{trip.stats.avgSpeed.toFixed(1)} km/h avg</span>
                      </div>
                    </div>
                    <div className="trip-actions">
                      <button className="view-btn" onClick={() => viewTrip(trip)}>
                        View
                      </button>
                      <button className="delete-small-btn" onClick={() => {
                        if (window.confirm('Delete this trip?')) {
                          deleteTrip(trip.id);
                        }
                      }}>
                        🗑️
                      </button>
                    </div>
                  </div>
                ))
              )}
            </div>
          </div>
        </div>
      )}
    </>
  );
}

export default App;
