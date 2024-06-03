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

  node_t *add(std::unique_ptr<node_t> n) {
    n->parent_ = this;
    children_.push_back(std::move(n));
    return children_.back().get();
  }
  void remove(node_t *n) {
    auto it = children_.begin();
    for (; it != children_.end(); ++it)
      if (it->get() == n)
        break;
    if (it != children_.end())
      children_.erase(it);
  }
  node_t *parent() { return parent_; }
  const node_t *parent() const { return parent_; }

  void listen(const sf::Event &e) {
    listen_this(e);
    for (auto &c : children_)
      c->listen(e);
  }
  void update() {
    update_this();
    for (auto &c : children_)
      c->update();
  }
  void draw(sf::RenderTarget &t, sf::RenderStates s) const override {
    this->render(t, s);
    for (auto &c : children_)
      c->draw(t, s);
  }

  virtual void render(sf::RenderTarget &, sf::RenderStates) const {}

  virtual void listen_this(const sf::Event &) {}
  virtual void update_this() {}

private:
  node_t *parent_;
  std::list<std::unique_ptr<node_t>> children_;
};
} // namespace bb
