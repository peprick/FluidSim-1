#include "FluidSimulation.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <iomanip>
#include <optional>
#include <sstream>

int main() {
    constexpr unsigned Width = 1280, Height = 800;
    sf::RenderWindow window(sf::VideoMode({Width, Height}), "FluidSim-1 | SPH Fluid Sandbox");
    window.setFramerateLimit(144);
    FluidSimulation simulation({{20.0F, 70.0F}, {static_cast<float>(Width - 40), static_cast<float>(Height - 90)}});
    sf::Clock clock;
    int brushColumns = 5;
    int brushRows = 5;
    float controlRepeat = 0.0F;
    constexpr float SimulationSpeed = 1.5F;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Space) simulation.setPaused(!simulation.paused());
                if (key->code == sf::Keyboard::Key::R) simulation.reset();
                if (key->code == sf::Keyboard::Key::Left) brushColumns = std::max(1, brushColumns - 2);
                if (key->code == sf::Keyboard::Key::Right) brushColumns = std::min(25, brushColumns + 2);
                if (key->code == sf::Keyboard::Key::Up) brushRows = std::min(25, brushRows + 2);
                if (key->code == sf::Keyboard::Key::Down) brushRows = std::max(1, brushRows - 2);
            }
            if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
                const auto mouse = sf::Vector2f(sf::Mouse::getPosition(window));
                if (click->button == sf::Mouse::Button::Left) simulation.addFluid(mouse, brushColumns, brushRows);
            }
        }
        const sf::Vector2f mouse = sf::Vector2f(sf::Mouse::getPosition(window));
        const float elapsed = std::min(clock.restart().asSeconds(), 0.05F);
        controlRepeat -= elapsed;
        if (controlRepeat <= 0.0F) {
            const bool increaseGravity = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
            const bool decreaseGravity = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
            const bool increaseViscosity = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
            const bool decreaseViscosity = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
            if (increaseGravity) simulation.settings().gravity += 25.0F;
            if (decreaseGravity) simulation.settings().gravity = std::max(0.0F, simulation.settings().gravity - 25.0F);
            if (increaseViscosity) simulation.settings().viscosity += 0.05F;
            if (decreaseViscosity) simulation.settings().viscosity = std::max(0.0F, simulation.settings().viscosity - 0.05F);
            if (increaseGravity || decreaseGravity || increaseViscosity || decreaseViscosity) controlRepeat = 0.08F;
        }
        simulation.update(elapsed * SimulationSpeed);

        window.clear(sf::Color(10, 18, 32));
        sf::RectangleShape tank({static_cast<float>(Width - 40), static_cast<float>(Height - 90)});
        tank.setPosition({20.0F, 70.0F}); tank.setFillColor(sf::Color(14, 31, 53)); tank.setOutlineThickness(2.0F); tank.setOutlineColor(sf::Color(70, 140, 195));
        window.draw(tank); simulation.draw(window);
        for (const SolidBlock& block : simulation.solidBlocks()) {
            sf::RectangleShape platform(block.bounds.size);
            platform.setPosition(block.bounds.position);
            platform.setFillColor(sf::Color(111, 81, 58));
            platform.setOutlineColor(sf::Color(235, 187, 119));
            platform.setOutlineThickness(2.0F);
            window.draw(platform);
        }
        sf::RectangleShape brushPreview({brushColumns * 7.0F, brushRows * 7.0F});
        brushPreview.setOrigin(brushPreview.getSize() / 2.0F);
        brushPreview.setPosition(mouse);
        brushPreview.setFillColor(sf::Color::Transparent);
        brushPreview.setOutlineColor(sf::Color(135, 220, 255, 180));
        brushPreview.setOutlineThickness(1.0F);
        window.draw(brushPreview);
        std::ostringstream title;
        title << "FluidSim-1 | " << simulation.particleCount() << " particles | gravity " << static_cast<int>(simulation.settings().gravity)
              << " | viscosity " << std::fixed << std::setprecision(2) << simulation.settings().viscosity
              << " | brush " << brushColumns << "x" << brushRows;
        window.setTitle(title.str());
        window.display();
    }
}
