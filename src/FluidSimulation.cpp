#include "FluidSimulation.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <numbers>
#include <random>
#include <thread>

namespace {
constexpr float FixedStep = 1.0F / 90.0F;
constexpr float Epsilon = 0.0001F;

float length(sf::Vector2f value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

sf::Vector2f normalize(sf::Vector2f value) {
    const float valueLength = length(value);
    return valueLength > Epsilon ? value / valueLength : sf::Vector2f{};
}

template <typename Work>
void parallelFor(std::size_t itemCount, Work&& work) {
    // Thread startup is more expensive than the SPH work for small scenes.
    if (itemCount < 2'000) {
        work(0, itemCount);
        return;
    }
    const unsigned hardwareThreads = std::max(1U, std::thread::hardware_concurrency());
    const unsigned threadCount = std::min<unsigned>(hardwareThreads, static_cast<unsigned>((itemCount + 1'499) / 1'500));
    if (threadCount <= 1) {
        work(0, itemCount);
        return;
    }

    std::vector<std::jthread> workers;
    workers.reserve(threadCount);
    const std::size_t chunkSize = (itemCount + threadCount - 1) / threadCount;
    for (unsigned thread = 0; thread < threadCount; ++thread) {
        const std::size_t begin = thread * chunkSize;
        const std::size_t end = std::min(itemCount, begin + chunkSize);
        if (begin < end) workers.emplace_back([begin, end, &work] { work(begin, end); });
    }
}
} // namespace

FluidSimulation::FluidSimulation(sf::FloatRect bounds) : bounds_(bounds) {
    reset();
}

void FluidSimulation::reset() {
    particles_.clear();
    solidBlocks_.clear();
    std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<float> width(110.0F, 210.0F);
    std::uniform_real_distribution<float> height(18.0F, 30.0F);
    std::uniform_real_distribution<float> xPosition(bounds_.position.x + 480.0F, bounds_.position.x + bounds_.size.x - 240.0F);
    std::uniform_real_distribution<float> yPosition(bounds_.position.y + 180.0F, bounds_.position.y + bounds_.size.y - 90.0F);
    for (int i = 0; i < 5; ++i) {
        const float blockWidth = width(generator);
        solidBlocks_.push_back({{{xPosition(generator), yPosition(generator)}, {blockWidth, height(generator)}}});
    }
    constexpr float spacing = 8.0F;
    for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 45; ++x) {
            particles_.push_back({{bounds_.position.x + 90.0F + x * spacing, bounds_.position.y + 80.0F + y * spacing}, {}, {}, settings_.restDensity, 0.0F});
        }
    }
}

void FluidSimulation::addFluid(sf::Vector2f center, int columns, int rows) {
    constexpr float spacing = 7.0F;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < columns; ++x) {
            const sf::Vector2f position{center.x + (x - columns / 2) * spacing, center.y + (y - rows / 2) * spacing};
            if (particles_.size() < MaxParticles && bounds_.contains(position)) {
                particles_.push_back({position, {}, {}, settings_.restDensity, 0.0F});
            }
        }
    }
}

void FluidSimulation::update(float deltaSeconds) {
    if (paused_) return;
    const int steps = std::clamp(static_cast<int>(std::ceil(deltaSeconds / FixedStep)), 1, 6);
    for (int i = 0; i < steps; ++i) {
        rebuildGrid();
        computeDensityAndPressure();
        computeForces();
        integrate(std::min(deltaSeconds / static_cast<float>(steps), FixedStep));
    }
}

void FluidSimulation::applyMouseForce(sf::Vector2f point, sf::Vector2f direction) {
    constexpr float radius = 90.0F;
    for (Particle& particle : particles_) {
        const sf::Vector2f offset = particle.position - point;
        const float distance = length(offset);
        if (distance < radius) particle.velocity += direction * (1.0F - distance / radius) * 0.012F;
    }
}

void FluidSimulation::rebuildGrid() {
    grid_.clear();
    grid_.reserve(particles_.size());
    for (std::size_t i = 0; i < particles_.size(); ++i) {
        const auto cell = cellOf(particles_[i].position);
        grid_[cellKey(cell.x, cell.y)].push_back(i);
    }
}

void FluidSimulation::computeDensityAndPressure() {
    parallelFor(particles_.size(), [this](std::size_t begin, std::size_t end) {
        std::vector<std::size_t> nearby;
        nearby.reserve(96);
        for (std::size_t particleIndex = begin; particleIndex < end; ++particleIndex) {
            Particle& particle = particles_[particleIndex];
            particle.density = 0.0F;
            collectNeighbors(particle.position, nearby);
            for (const std::size_t index : nearby) {
                const sf::Vector2f offset = particle.position - particles_[index].position;
                particle.density += settings_.particleMass * poly6(offset.x * offset.x + offset.y * offset.y);
            }
            particle.density = std::max(particle.density, 0.01F);
            particle.pressure = settings_.pressureStiffness * std::max(0.0F, particle.density - settings_.restDensity);
        }
    });
}

void FluidSimulation::computeForces() {
    parallelFor(particles_.size(), [this](std::size_t begin, std::size_t end) {
        std::vector<std::size_t> nearby;
        nearby.reserve(96);
        for (std::size_t particleIndex = begin; particleIndex < end; ++particleIndex) {
            Particle& particle = particles_[particleIndex];
            sf::Vector2f pressureForce{};
            sf::Vector2f viscosityForce{};
            collectNeighbors(particle.position, nearby);
            for (const std::size_t index : nearby) {
                const Particle& other = particles_[index];
                const sf::Vector2f offset = particle.position - other.position;
                const float distance = length(offset);
                if (distance <= Epsilon || distance >= settings_.smoothingRadius) continue;
                const sf::Vector2f gradient = spikyGradient(offset, distance);
                const float sharedPressure = (particle.pressure + other.pressure) / (2.0F * other.density);
                pressureForce -= gradient * settings_.particleMass * sharedPressure;
                viscosityForce += (other.velocity - particle.velocity) * (settings_.viscosity * settings_.particleMass / other.density * viscosityLaplacian(distance));
            }
            particle.force = pressureForce + viscosityForce + sf::Vector2f{0.0F, settings_.gravity * particle.density};
        }
    });
}

void FluidSimulation::integrate(float deltaSeconds) {
    const float floor = bounds_.position.y + bounds_.size.y - settings_.particleRadius;
    parallelFor(particles_.size(), [this, deltaSeconds, floor](std::size_t begin, std::size_t end) {
        for (std::size_t particleIndex = begin; particleIndex < end; ++particleIndex) {
            Particle& particle = particles_[particleIndex];
            particle.velocity += particle.force / particle.density * deltaSeconds;
            particle.position += particle.velocity * deltaSeconds;
            constrainToBounds(particle);
            const bool restingOnFloor = particle.position.y >= floor - 0.1F && length(particle.velocity) < 18.0F;
            particle.settledTime = restingOnFloor ? particle.settledTime + deltaSeconds : 0.0F;
        }
    });
    // A settled floor particle cannot affect future visible motion, so discard it.
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& particle) {
        return particle.settledTime > 0.45F;
    }), particles_.end());
}

void FluidSimulation::constrainToBounds(Particle& particle) {
    const float r = settings_.particleRadius;
    const float left = bounds_.position.x + r, right = bounds_.position.x + bounds_.size.x - r;
    const float top = bounds_.position.y + r, bottom = bounds_.position.y + bounds_.size.y - r;
    if (particle.position.x < left) { particle.position.x = left; particle.velocity.x *= -settings_.boundaryDamping; }
    if (particle.position.x > right) { particle.position.x = right; particle.velocity.x *= -settings_.boundaryDamping; }
    if (particle.position.y < top) { particle.position.y = top; particle.velocity.y *= -settings_.boundaryDamping; }
    if (particle.position.y > bottom) { particle.position.y = bottom; particle.velocity.y *= -settings_.boundaryDamping; }

    for (const SolidBlock& block : solidBlocks_) {
        const sf::Vector2f min = block.bounds.position;
        const sf::Vector2f max = block.bounds.position + block.bounds.size;
        sf::Vector2f contact{std::clamp(particle.position.x, min.x, max.x), std::clamp(particle.position.y, min.y, max.y)};
        sf::Vector2f separation = particle.position - contact;
        float distance = length(separation);
        if (distance >= r) continue;

        // When a particle is inside a block, push it out through the closest face.
        if (distance < Epsilon) {
            const float toLeft = particle.position.x - min.x, toRight = max.x - particle.position.x;
            const float toTop = particle.position.y - min.y, toBottom = max.y - particle.position.y;
            const float closest = std::min({toLeft, toRight, toTop, toBottom});
            if (closest == toLeft) { separation = {-1.0F, 0.0F}; contact.x = min.x; }
            else if (closest == toRight) { separation = {1.0F, 0.0F}; contact.x = max.x; }
            else if (closest == toTop) { separation = {0.0F, -1.0F}; contact.y = min.y; }
            else { separation = {0.0F, 1.0F}; contact.y = max.y; }
            distance = 0.0F;
        } else separation /= distance;

        const sf::Vector2f normal = normalize(separation);
        particle.position = contact + normal * r;
        const float normalVelocity = particle.velocity.x * normal.x + particle.velocity.y * normal.y;
        if (normalVelocity < 0.0F) particle.velocity -= normal * (1.0F + settings_.boundaryDamping) * normalVelocity;
    }
}

void FluidSimulation::draw(sf::RenderTarget& target) const {
    sf::VertexArray vertices(sf::PrimitiveType::Triangles, particles_.size() * 6);
    const float r = settings_.particleRadius;
    std::size_t vertex = 0;
    for (const Particle& particle : particles_) {
        const float normalized = std::clamp(particle.density / (settings_.restDensity * 2.0F), 0.0F, 1.0F);
        const sf::Color color(40, static_cast<std::uint8_t>(130 + 80 * normalized), 255, 220);
        const sf::Vector2f a = particle.position + sf::Vector2f{-r, -r};
        const sf::Vector2f b = particle.position + sf::Vector2f{r, -r};
        const sf::Vector2f c = particle.position + sf::Vector2f{r, r};
        const sf::Vector2f d = particle.position + sf::Vector2f{-r, r};
        vertices[vertex++] = {a, color}; vertices[vertex++] = {b, color}; vertices[vertex++] = {c, color};
        vertices[vertex++] = {a, color}; vertices[vertex++] = {c, color}; vertices[vertex++] = {d, color};
    }
    target.draw(vertices);
}

void FluidSimulation::collectNeighbors(sf::Vector2f position, std::vector<std::size_t>& result) const {
    result.clear();
    const auto cell = cellOf(position);
    for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) {
        if (const auto it = grid_.find(cellKey(cell.x + x, cell.y + y)); it != grid_.end()) result.insert(result.end(), it->second.begin(), it->second.end());
    }
}

long long FluidSimulation::cellKey(int x, int y) const { return (static_cast<long long>(x) << 32) ^ static_cast<unsigned int>(y); }
sf::Vector2i FluidSimulation::cellOf(sf::Vector2f position) const { return {static_cast<int>(std::floor(position.x / settings_.smoothingRadius)), static_cast<int>(std::floor(position.y / settings_.smoothingRadius))}; }
float FluidSimulation::poly6(float r2) const { const float h2 = settings_.smoothingRadius * settings_.smoothingRadius; if (r2 >= h2) return 0.0F; const float d = h2 - r2; return 4.0F * d * d * d / (std::numbers::pi_v<float> * std::pow(settings_.smoothingRadius, 8.0F)); }
sf::Vector2f FluidSimulation::spikyGradient(sf::Vector2f displacement, float distance) const { const float d = settings_.smoothingRadius - distance; return normalize(displacement) * (-30.0F * d * d / (std::numbers::pi_v<float> * std::pow(settings_.smoothingRadius, 5.0F))); }
float FluidSimulation::viscosityLaplacian(float distance) const { return 20.0F * (settings_.smoothingRadius - distance) / (std::numbers::pi_v<float> * std::pow(settings_.smoothingRadius, 5.0F)); }
