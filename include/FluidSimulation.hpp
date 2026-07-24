#pragma once

#include <SFML/Graphics.hpp>
#include <cstddef>
#include <vector>

struct SimulationSettings {
    float smoothingRadius = 18.0F;
    float particleMass = 1.0F;
    float restDensity = 7.0F;
    float pressureStiffness = 180.0F;
    float viscosity = 0.8F;
    float gravity = 650.0F;
    float boundaryDamping = 0.45F;
    float particleRadius = 3.5F;
};

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f force;
    float density = 0.0F;
    float pressure = 0.0F;
    float settledTime = 0.0F;
};

struct SolidBlock {
    sf::FloatRect bounds;
};

class FluidSimulation {
public:
    explicit FluidSimulation(sf::FloatRect bounds);

    void reset();
    void update(float deltaSeconds);
    void addFluid(sf::Vector2f center, int columns = 5, int rows = 5);
    void applyMouseForce(sf::Vector2f point, sf::Vector2f direction);
    void draw(sf::RenderTarget& target) const;

    void setPaused(bool paused) { paused_ = paused; }
    [[nodiscard]] bool paused() const { return paused_; }
    [[nodiscard]] const SimulationSettings& settings() const { return settings_; }
    SimulationSettings& settings() { return settings_; }
    [[nodiscard]] std::size_t particleCount() const { return particles_.size(); }
    [[nodiscard]] const std::vector<SolidBlock>& solidBlocks() const { return solidBlocks_; }

private:
    using Grid = std::vector<std::vector<std::size_t>>;

    void rebuildGrid();
    void computeDensityAndPressure();
    void computeForces();
    void integrate(float deltaSeconds);
    void constrainToBounds(Particle& particle);
    void collectNeighbors(sf::Vector2f position, std::vector<std::size_t>& result) const;
    [[nodiscard]] sf::Vector2i cellOf(sf::Vector2f position) const;
    [[nodiscard]] std::size_t cellIndex(int x, int y) const;
    [[nodiscard]] float poly6(float distanceSquared) const;
    [[nodiscard]] sf::Vector2f spikyGradient(sf::Vector2f displacement, float distance) const;
    [[nodiscard]] float viscosityLaplacian(float distance) const;

    sf::FloatRect bounds_;
    SimulationSettings settings_;
    std::vector<Particle> particles_;
    std::vector<SolidBlock> solidBlocks_;
    Grid grid_;
    int gridColumns_ = 0;
    int gridRows_ = 0;
    float poly6Coefficient_ = 0.0F;
    float spikyGradientCoefficient_ = 0.0F;
    float viscosityLaplacianCoefficient_ = 0.0F;
    bool paused_ = false;
    static constexpr std::size_t MaxParticles = 6000;
};
