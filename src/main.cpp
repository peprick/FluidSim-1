#include "FluidSimulation.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

int main() {
    constexpr unsigned Width = 1280, Height = 800;
    constexpr float TankTop = 112.0F;
    constexpr float TankHeight = 630.0F;
    constexpr sf::FloatRect TankBounds{{20.0F, TankTop}, {static_cast<float>(Width - 40), TankHeight}};
    sf::RenderWindow window(sf::VideoMode({Width, Height}), "FluidSim-1 | SPH Fluid Sandbox");
    window.setFramerateLimit(144);
    FluidSimulation simulation(TankBounds);
    sf::Font font;
    const bool hasFont = font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");
    sf::Clock clock;
    int brushColumns = 5;
    int brushRows = 5;
    float controlRepeat = 0.0F;
    float displayedFps = 0.0F;
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
        if (elapsed > 0.0F) displayedFps = displayedFps * 0.9F + (1.0F / elapsed) * 0.1F;
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

        window.clear(sf::Color(7, 14, 28));

        sf::RectangleShape header({static_cast<float>(Width), 92.0F});
        header.setFillColor(sf::Color(13, 28, 49));
        window.draw(header);
        sf::RectangleShape accent({static_cast<float>(Width), 3.0F});
        accent.setPosition({0.0F, 89.0F});
        accent.setFillColor(sf::Color(48, 202, 255));
        window.draw(accent);

        sf::RectangleShape tank(TankBounds.size);
        tank.setPosition(TankBounds.position);
        tank.setFillColor(sf::Color(12, 32, 55));
        tank.setOutlineThickness(2.0F);
        tank.setOutlineColor(sf::Color(54, 132, 184));
        window.draw(tank);

        // Draw subtle grid lines once per frame to give the tank visual scale.
        sf::VertexArray grid(sf::PrimitiveType::Lines);
        for (float x = TankBounds.position.x + 40.0F; x < TankBounds.position.x + TankBounds.size.x; x += 40.0F) {
            grid.append({{x, TankBounds.position.y}, sf::Color(90, 162, 205, 20)});
            grid.append({{x, TankBounds.position.y + TankBounds.size.y}, sf::Color(90, 162, 205, 20)});
        }
        for (float y = TankBounds.position.y + 40.0F; y < TankBounds.position.y + TankBounds.size.y; y += 40.0F) {
            grid.append({{TankBounds.position.x, y}, sf::Color(90, 162, 205, 20)});
            grid.append({{TankBounds.position.x + TankBounds.size.x, y}, sf::Color(90, 162, 205, 20)});
        }
        window.draw(grid);
        simulation.draw(window);
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
        brushPreview.setFillColor(sf::Color(80, 212, 255, 22));
        brushPreview.setOutlineColor(TankBounds.contains(mouse) ? sf::Color(135, 220, 255, 220) : sf::Color(255, 255, 255, 50));
        brushPreview.setOutlineThickness(1.5F);
        if (TankBounds.contains(mouse)) window.draw(brushPreview);

        if (hasFont) {
            sf::Text heading(font, "FLUIDSIM-1", 25);
            heading.setPosition({24.0F, 14.0F});
            heading.setFillColor(sf::Color(235, 248, 255));
            heading.setStyle(sf::Text::Bold);
            window.draw(heading);
            sf::Text subtitle(font, "Interactive SPH particle sandbox", 14);
            subtitle.setPosition({26.0F, 47.0F});
            subtitle.setFillColor(sf::Color(138, 181, 208));
            window.draw(subtitle);

            const std::array<std::string, 4> stats{
                std::to_string(simulation.particleCount()) + " particles",
                "gravity " + std::to_string(static_cast<int>(simulation.settings().gravity)),
                "viscosity " + [&] { std::ostringstream value; value << std::fixed << std::setprecision(2) << simulation.settings().viscosity; return value.str(); }(),
                std::to_string(static_cast<int>(displayedFps)) + " FPS"
            };
            for (std::size_t i = 0; i < stats.size(); ++i) {
                const float x = 455.0F + static_cast<float>(i) * 195.0F;
                sf::RectangleShape chip({176.0F, 34.0F});
                chip.setPosition({x, 28.0F});
                chip.setFillColor(sf::Color(25, 52, 79));
                chip.setOutlineColor(i == 3 ? sf::Color(60, 220, 166) : sf::Color(53, 96, 128));
                chip.setOutlineThickness(1.0F);
                window.draw(chip);
                sf::Text label(font, stats[i], 14);
                label.setPosition({x + 12.0F, 36.0F});
                label.setFillColor(sf::Color(213, 235, 247));
                window.draw(label);
            }

            sf::Text footer(font, "CLICK  spawn fluid     ARROWS  resize brush     W / S  gravity     A / D  viscosity     SPACE  pause     R  reset", 14);
            footer.setPosition({28.0F, 765.0F});
            footer.setFillColor(sf::Color(148, 184, 208));
            window.draw(footer);
            if (simulation.paused()) {
                sf::Text paused(font, "PAUSED", 20);
                paused.setPosition({TankBounds.position.x + 18.0F, TankBounds.position.y + 16.0F});
                paused.setFillColor(sf::Color(255, 218, 118));
                paused.setStyle(sf::Text::Bold);
                window.draw(paused);
            }
        }
        std::ostringstream title;
        title << "FluidSim-1 | " << simulation.particleCount() << " particles | gravity " << static_cast<int>(simulation.settings().gravity)
              << " | viscosity " << std::fixed << std::setprecision(2) << simulation.settings().viscosity
              << " | brush " << brushColumns << "x" << brushRows;
        window.setTitle(title.str());
        window.display();
    }
}
