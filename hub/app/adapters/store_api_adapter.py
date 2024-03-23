import json
import logging
from typing import List

import pydantic_core
import requests
from datetime import datetime

from app.entities.processed_agent_data import ProcessedAgentData
from app.interfaces.store_gateway import StoreGateway

class StoreApiAdapter(StoreGateway):
    def __init__(self, api_base_url):
        self.api_base_url = api_base_url

    def save_data(self, processed_data_batch: List[ProcessedAgentData]):
        url = f"{self.api_base_url}/processed_agent_data/"

        headers = {'Content-Type': 'application/json'}

        data = []
        for processed_agent_data in processed_data_batch:
            timestamp = processed_agent_data.agent_data.timestamp.strftime("%Y-%m-%dT%H:%M:%S.%fZ")
            processed_agent_data.agent_data.timestamp = timestamp
            data.append(processed_agent_data.dict())

        try:
            with requests.post(url, data=json.dumps(data), headers=headers) as response:
                if response.status_code != 200:
                    logging.error(f"Invalid Hub response\nData: {data}\nResponse: {response}")
                    return False
        except Exception as e:
            logging.error(f"Error occurred during request: {e}")
            return False
        return True
