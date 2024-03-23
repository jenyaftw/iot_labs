import logging
from typing import List

from fastapi import FastAPI, HTTPException
from redis import Redis
import paho.mqtt.client as mqtt
import scipy

from app.adapters.store_api_adapter import StoreApiAdapter
from app.entities.processed_agent_data import ProcessedAgentData
from app.entities.agent_data import AgentData
from config import (
    STORE_API_BASE_URL,
    REDIS_HOST,
    REDIS_PORT,
    BATCH_SIZE,
    MQTT_TOPIC,
    MQTT_BROKER_HOST,
    MQTT_BROKER_PORT,
)

# Configure logging settings
logging.basicConfig(
    level=logging.INFO,  # Set the log level to INFO (you can use logging.DEBUG for more detailed logs)
    format="[%(asctime)s] [%(levelname)s] [%(module)s] %(message)s",
    handlers=[
        logging.StreamHandler(),  # Output log messages to the console
        logging.FileHandler("app.log"),  # Save log messages to a file
    ],
)
# Create an instance of the Redis using the configuration
redis_client = Redis(host=REDIS_HOST, port=REDIS_PORT)
# Create an instance of the StoreApiAdapter using the configuration
store_adapter = StoreApiAdapter(api_base_url=STORE_API_BASE_URL)
# Create an instance of the AgentMQTTAdapter using the configuration

# FastAPI
app = FastAPI()


@app.post("/agent_data/")
async def process_and_save_agent_data(agent_data: AgentData):
    redis_client.lpush("agent_data", agent_data.model_dump_json())
    if redis_client.llen("agent_data") >= BATCH_SIZE:
        agent_data_batch: List[ProcessedAgentData] = []
        for _ in range(BATCH_SIZE):
            agent_data = AgentData.model_validate_json(
                redis_client.lpop("agent_data")
            )
            agent_data_batch.append(agent_data)
        
        processed_data_batch = process_agent_data(agent_data_batch)
        store_adapter.save_data(processed_data_batch=processed_data_batch)
    return {"status": "ok"}

def process_agent_data(agent_data_batch: List[AgentData]):
    processed_data_batch = []
    
    z_values = list(map(lambda item: item.accelerometer.z, agent_data_batch))
    bumps_indices, _ = scipy.signal.find_peaks(z_values, prominence=7000, width=3)
    bumps = list(map(
        lambda i: agent_data_batch[i].gps,
        bumps_indices
    ))
        
    inverted_z_values = list(map(lambda z: -z, z_values))
    potholes_indices, _ = scipy.signal.find_peaks(inverted_z_values, prominence=7000, width=3)
    potholes = list(map(
        lambda i: agent_data_batch[i].gps,
        potholes_indices
    ))
    
    for i, val in enumerate(agent_data_batch):
        road_state = "normal"
        if i in bumps_indices:
            road_state = "bump"
        if i in potholes_indices:
            road_state = "pothole"

        processed_data_batch.append(
            ProcessedAgentData(road_state=road_state, agent_data=val)
        )
    
    return processed_data_batch
    

# MQTT
client = mqtt.Client()


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        logging.info("Connected to MQTT broker")
        client.subscribe(MQTT_TOPIC)
    else:
        logging.info(f"Failed to connect to MQTT broker with code: {rc}")


def on_message(client, userdata, msg):
    try:
        payload: str = msg.payload.decode("utf-8")
        # Create ProcessedAgentData instance with the received data
        agent_data = AgentData.model_validate_json(
            payload, strict=True
        )

        redis_client.lpush(
            "agent_data", agent_data.model_dump_json()
        )
        agent_data_batch: List[AgentData] = []
        if redis_client.llen("agent_data") >= BATCH_SIZE:
            for _ in range(BATCH_SIZE):
                agent_data = AgentData.model_validate_json(
                    redis_client.lpop("agent_data")
                )
                agent_data_batch.append(agent_data)

            processed_data_batch = process_agent_data(agent_data_batch)
            store_adapter.save_data(processed_data_batch=processed_data_batch)
        return {"status": "ok"}
    except Exception as e:
        logging.info(f"Error processing MQTT message: {e}")


# Connect
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT)

# Start
client.loop_start()
