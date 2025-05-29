from pydantic import BaseModel
from app.entities.agent_data import AgentData

class ProcessedAgentData(BaseModel):
    agent_data: AgentData
