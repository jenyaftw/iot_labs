import logging

import requests as requests

from app.entities.agent_data import AgentData
from app.interfaces.hub_gateway import HubGateway

class HubHttpAdapter(HubGateway):
    def __init__(self, api_base_url):
        self.api_base_url = api_base_url

    def save_data(self, agent_data: AgentData):
        """
        Save agent data to the Hub.
        Parameters:
            agent_data (AgentData): Agent data to be processed and saved.
        Returns:
            bool: True if the data is successfully saved, False otherwise.
        """
        url = f"{self.api_base_url}/agent_data/"

        response = requests.post(url, data=agent_data.model_dump_json())
        if response.status_code != 200:
            logging.info(
                f"Invalid Hub response\nData: {agent_data.model_dump_json()}\nResponse: {response}"
            )
            return False
        return True
