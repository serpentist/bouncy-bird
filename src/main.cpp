#include <SFML/Graphics.hpp>
#include <memory>

namespace bb {
std::unique_ptr<sf::RenderWindow> create_window(unsigned w, unsigned h);
}

int main() {
  auto window = bb::create_window(640, 480);
  while (window->isOpen()) {
    sf::Event event;
    while (window->pollEvent(event)) {
      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Escape)
        window->close();
    }

    window->clear();
    window->display();
  }

  return 0;
}

namespace bb {
std::unique_ptr<sf::RenderWindow> create_window(unsigned w, unsigned h) {
  return std::unique_ptr<sf::RenderWindow>{new sf::RenderWindow{
      sf::VideoMode{w, h}, "Bouncy Bird", sf::Style::None}};
}
} // namespace bb
