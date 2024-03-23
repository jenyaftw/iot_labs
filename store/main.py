import asyncio
import json
from typing import Set, Dict, List, Any
from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect, Body
from sqlalchemy import (
    create_engine,
    MetaData,
    Table,
    Column,
    Integer,
    String,
    Float,
    DateTime,
)
from sqlalchemy.orm import sessionmaker
from sqlalchemy.sql import select, delete, update
from datetime import datetime
from pydantic import BaseModel, field_validator
from config import (
    POSTGRES_HOST,
    POSTGRES_PORT,
    POSTGRES_DB,
    POSTGRES_USER,
    POSTGRES_PASSWORD,
)

# FastAPI app setup
app = FastAPI()
# SQLAlchemy setup
DATABASE_URL = f"postgresql+psycopg2://{POSTGRES_USER}:{POSTGRES_PASSWORD}@{POSTGRES_HOST}:{POSTGRES_PORT}/{POSTGRES_DB}"
engine = create_engine(DATABASE_URL)
metadata = MetaData()
# Define the ProcessedAgentData table
processed_agent_data = Table(
    "processed_agent_data",
    metadata,
    Column("id", Integer, primary_key=True, index=True),
    Column("road_state", String),
    Column("user_id", Integer),
    Column("x", Float),
    Column("y", Float),
    Column("z", Float),
    Column("latitude", Float),
    Column("longitude", Float),
    Column("timestamp", DateTime),
)
SessionLocal = sessionmaker(bind=engine)
metadata.create_all(engine)

# SQLAlchemy model
class ProcessedAgentDataInDB(BaseModel):
    id: int
    road_state: str
    user_id: int
    x: float
    y: float
    z: float
    latitude: float
    longitude: float
    timestamp: datetime


# FastAPI models
class AccelerometerData(BaseModel):
    x: float
    y: float
    z: float


class GpsData(BaseModel):
    latitude: float
    longitude: float


class AgentData(BaseModel):
    user_id: int
    accelerometer: AccelerometerData
    gps: GpsData
    timestamp: datetime

    @classmethod
    @field_validator("timestamp", mode="before")
    def check_timestamp(cls, value):
        if isinstance(value, datetime):
            return value
        try:
            return datetime.fromisoformat(value)
        except (TypeError, ValueError):
            raise ValueError(
                "Invalid timestamp format. Expected ISO 8601 format (YYYY-MM-DDTHH:MM:SSZ)."
            )

class ProcessedAgentData(BaseModel):
    road_state: str
    agent_data: AgentData

# WebSocket subscriptions
subscriptions: Set[WebSocket] = set()

# FastAPI WebSocket endpoint
@app.websocket("/ws/")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    subscriptions.add(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        subscriptions.remove(websocket)

# Function to send data to subscribed users
async def send_data_to_subscribers(data: List[ProcessedAgentData]):
    for websocket in subscriptions:
        await websocket.send_json(data)

# FastAPI CRUDL endpoints

@app.post("/processed_agent_data/")
async def create_processed_agent_data(data: List[ProcessedAgentData]):
    # Insert data to database
    print("Creating processed agent data...")

    with SessionLocal() as session:
        for item in data:
            query = processed_agent_data.insert().values(
                road_state=item.road_state,
                user_id=item.agent_data.user_id,
                x=item.agent_data.accelerometer.x,
                y=item.agent_data.accelerometer.y,
                z=item.agent_data.accelerometer.z,
                latitude=item.agent_data.gps.latitude,
                longitude=item.agent_data.gps.longitude,
                timestamp=item.agent_data.timestamp,
            )
            session.execute(query)

        session.commit()
        print("Success!")
        
    # Send data to subscribers
    await send_data_to_subscribers(data)

@app.get(
    "/processed_agent_data/{processed_agent_data_id}",
    response_model=ProcessedAgentDataInDB,
)
def read_processed_agent_data(processed_agent_data_id: int):
    # Get data by id

    print("Reading processed agent data...")

    with SessionLocal() as session:
        # print(processed_agent_data_id)
        query = select(processed_agent_data).where(
            processed_agent_data.c.id == processed_agent_data_id
        )

        result = session.execute(query).first()
        print("Result: ", result)

        if result is None:
            print("Data not found")
            raise HTTPException(status_code=404, detail="Data not found")

        print("Success!")
        return result


@app.get("/processed_agent_data/", response_model=list[ProcessedAgentDataInDB])
def list_processed_agent_data():
    # Get list of data

    print("Listing processed agent data...")

    with SessionLocal() as session:
        query = select(processed_agent_data)

        result = session.execute(query)
        print("Result: ", result)

        if result is None:
            print("Data not found")
            raise HTTPException(status_code=404, detail="Data not found")

        print("Success!")
        return result


@app.put(
    "/processed_agent_data/{processed_agent_data_id}",
    response_model=ProcessedAgentDataInDB,
)
def update_processed_agent_data(processed_agent_data_id: int, data: ProcessedAgentData):
    # Update data

    print("Updating processed agent data...")

    with SessionLocal() as session:
        query = select(processed_agent_data).where(
            processed_agent_data.c.id == processed_agent_data_id)

        result = session.execute(query).first()
        print("Result: ", result)

        if result is None:
            print("Data not found")
            raise HTTPException(status_code=404, detail="Data not found")

        query = update(processed_agent_data).where(
            processed_agent_data.c.id == processed_agent_data_id
        ).values(
            road_state=data.road_state,
            user_id=data.agent_data.user_id,
            x=data.agent_data.accelerometer.x,
            y=data.agent_data.accelerometer.y,
            z=data.agent_data.accelerometer.z,
            latitude=data.agent_data.gps.latitude,
            longitude=data.agent_data.gps.latitude,
            timestamp=data.agent_data.timestamp,
        )

        session.execute(query)
        session.commit()

        query = select(processed_agent_data).where(
            processed_agent_data.c.id == processed_agent_data_id)
        result = session.execute(query).first()
        print("Result: ", result)

        print("Success!")
        return result


@app.delete(
    "/processed_agent_data/{processed_agent_data_id}",
    response_model=ProcessedAgentDataInDB,
)
def delete_processed_agent_data(processed_agent_data_id: int):
    # Delete by id

    print("Deleting processed agent data...")

    with SessionLocal() as session:
        query = select(processed_agent_data).where(
            processed_agent_data.c.id == processed_agent_data_id)
        result = session.execute(query).first()
        print("Result: ", result)

        if result is None:
            print("Data not found")
            raise HTTPException(status_code=404, detail="Data not found")

        query = delete(processed_agent_data).where(
            processed_agent_data.c.id == processed_agent_data_id
        )

        session.execute(query)
        session.commit()
        print("Success!")
        return result


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="127.0.0.1", port=8000)
