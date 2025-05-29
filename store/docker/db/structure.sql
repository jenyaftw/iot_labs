CREATE TABLE processed_agent_data (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    x FLOAT,
    y FLOAT,
    z FLOAT,
    latitude FLOAT,
    longitude FLOAT,
    aqi INTEGER,
    rssi INTEGER,
    timestamp TIMESTAMP
);
