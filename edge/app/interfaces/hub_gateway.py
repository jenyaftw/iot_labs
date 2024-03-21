from abc import ABC, abstractmethod
from app.entities.processed_agent_data import ProcessedAgentData

class HubGateway(ABC):
    """
    Abstract class representing the Store Gateway interface.
    All store gateway adapters must implement these methods.
    """

    @abstractmethod
    def save_data(self, agent_data: AgentData) -> bool:
        """
        Method to save the agent data in the database.
        Parameters:
            agent_data (AgentData): The agent data to be saved.
        Returns:
            bool: True if the data is successfully saved, False otherwise.
        """
        pass
