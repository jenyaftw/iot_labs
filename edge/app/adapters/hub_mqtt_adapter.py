import logging

import requests as requests
from paho.mqtt import client as mqtt_client

from app.entities.agent_data import AgentData
from app.interfaces.hub_gateway import HubGateway


class HubMqttAdapter(HubGateway):
    def __init__(self, broker, port, topic):
        self.broker = broker
        self.port = port
        self.topic = topic
        self.mqtt_client = self._connect_mqtt(broker, port)

    def save_data(self, agent_data: AgentData):
        """
        Save agent data to the Hub.
        Parameters:
            agent_data (AgentData): Agent data to be processed and saved.
        Returns:
            bool: True if the data is successfully saved, False otherwise.
        """
        msg = agent_data.model_dump_json()
        result = self.mqtt_client.publish(self.topic, msg)
        status = result[0]
        if status == 0:
            return True
        else:
            print(f"Failed to send message to topic {self.topic}")
            return False

    @staticmethod
    def _connect_mqtt(broker, port):
        """Create MQTT client"""
        print(f"CONNECT TO {broker}:{port}")

        def on_connect(client, userdata, flags, rc):
            if rc == 0:
                print(f"Connected to MQTT Broker ({broker}:{port})!")
            else:
                print("Failed to connect {broker}:{port}, return code %d\n", rc)
                exit(rc)  # Stop execution

        client = mqtt_client.Client()
        client.on_connect = on_connect
        client.connect(broker, port)
        client.loop_start()
        return client
