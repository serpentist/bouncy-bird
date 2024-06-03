#pragma once

#include <SFML/Graphics.hpp>
#include <list>

namespace bb {
class node_t : public sf::Drawable {
public:
  node_t() = default;
  virtual ~node_t() = default;
  node_t &operator=(const node_t &) = delete;
  node_t(const node_t &) = delete;
  node_t &operator=(node_t &&) = default;
  node_t(node_t &&) = default;

  node_t *add(node_t n) {
    n.parent_ = this;
    children_.push_back(std::move(n));
    return &children_.back();
  }
  void remove(node_t *n) {
    auto it = children_.begin();
    for (; it != children_.end(); ++it)
      if (&(*it) == n)
        break;
    if (it != children_.end()) {
      it->parent_ = nullptr;
      children_.erase(it);
    }
  }
  node_t *parent() { return parent_; }
  const node_t *parent() const { return parent_; }

  void listen(const sf::Event &e) {
    listen_this(e);
    for (auto &c : children_)
      c.listen(e);
  }
  void update() {
    update_this();
    for (auto &c : children_)
      c.update();
  }
  void render(sf::RenderTarget &t, sf::RenderStates s) {
    this->draw(t, s);
    for (auto &c : children_)
      c.render(t, s);
  }

  virtual void listen_this(const sf::Event &) {}
  virtual void update_this() {}
  void draw(sf::RenderTarget &, sf::RenderStates) const override {}

private:
  node_t *parent_;
  std::list<node_t> children_;
};
} // namespace bb
