#pragma once

struct SimulationParams
{
    float worldWidth{1000.0f};
    float worldHeight{1000.0f};

    float deltaTime{0.016f};

    float maxSpeed{120.0f};
    float maxForce{80.0f};

    float neighborRadius{40.0f};
    float separationRadius{15.0f};

    float separationWeight{1.5f};
    float alignmentWeight{1.0f};
    float cohesionWeight{1.0f};

    bool useWrapAround{true};

    int initialAgentCount{1000};
    int maxAgentCapacity{100000};

    float gridCellSize{40.0f};
};