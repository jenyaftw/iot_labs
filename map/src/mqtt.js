import mqtt from 'mqtt';
const websocketUrl = 'ws://b98d7441cb334792add9e2adab7d2292.s1.eu.hivemq.cloud:8884/mqtt';

function getClient(errorHandler, username, password) {
  const options = {
    username: username,
    password: password,
    protocol: 'wss', // Use secure WebSocket
    // Add client certificates if needed
    // cert: fs.readFileSync('client-cert.pem'),
    // key: fs.readFileSync('client-key.pem'),
    // ca: fs.readFileSync('ca-cert.pem'),
    connectTimeout: 60 * 1000, // 60 seconds
    keepalive: 60,
    clean: true,
  };

  const client = mqtt.connect(websocketUrl, options);

  client.stream.on('error', (err) => {
    errorHandler(`Connection to ${websocketUrl} failed`);
    client.end();
  });

  // Add connection success/failure handlers
  client.on('connect', () => {
    console.log('Connected to HiveMQ Cloud');
  });

  client.on('error', (err) => {
    errorHandler(`MQTT connection error: ${err.message}`);
  });

  return client;
}

function subscribe(client, topic, errorHandler) {
  const callBack = (err, granted) => {
    if (err) {
      errorHandler('Subscription request failed');
    }
  };
  return client.subscribe(topic, callBack);
}

function onMessage(client, callBack) {
  client.on('message', (topic, message, packet) => {
    let parsedMessage;
    try {
      // Try to parse as JSON for structured data
      parsedMessage = JSON.parse(new TextDecoder('utf-8').decode(message));
    } catch (e) {
      // If parsing fails, treat as plain text (for status and rssi topics)
      parsedMessage = new TextDecoder('utf-8').decode(message);
    }
    callBack(topic, parsedMessage);
  });
}

function unsubscribe(client, topic) {
  client.unsubscribe(topic);
}

function closeConnection(client) {
  client.end();
}

const mqttService = {
  getClient,
  subscribe,
  onMessage,
  unsubscribe,
  closeConnection,
};
export default mqttService;
