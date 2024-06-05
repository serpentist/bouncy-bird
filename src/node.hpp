#pragma once

#include <SFML/Graphics.hpp>
#include <chrono>
#include <list>
#include <random>

namespace bb {
class node_t : public sf::Drawable {
public:
  node_t() : visible_{true}, frozen_{false} {}
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
    if (!frozen_)
      listen_this(e);
    for (auto &c : children_)
      c->listen(e);
  }
  void update() {
    if (!frozen_)
      update_this();
    for (auto &c : children_)
      c->update();
  }
  void draw(sf::RenderTarget &t, sf::RenderStates s) const override {
    if (visible_)
      this->render(t, s);
    for (auto &c : children_)
      c->draw(t, s);
  }

  virtual void render(sf::RenderTarget &, sf::RenderStates) const {}

  virtual void listen_this(const sf::Event &) {}
  virtual void update_this() {}

  bool visible() const { return visible_; }
  bool frozen() const { return frozen_; }
  void visible(bool x) { visible_ = x; }
  void frozen(bool x) { frozen_ = x; }

private:
  node_t *parent_;
  std::list<std::unique_ptr<node_t>> children_;
  bool visible_;
  bool frozen_;
};
} // namespace bb

namespace bb {
class game_t : public node_t {
public:
  void step_parameters(double a, double b, double c) {
    framerate_ = a;
    velocity_ = b;
    position_ = c;
  }
  void update_this() override {
    world_->Step(framerate_, velocity_, position_);
  }
  void initialize_world(float gravity) {
    world_ = std::unique_ptr<b2World>(new b2World(b2Vec2{0, gravity}));
  }
  b2World *world() { return world_.get(); }

private:
  double framerate_;
  double velocity_;
  double position_;
  std::unique_ptr<b2World> world_;
};
} // namespace bb

namespace bb {
class bird_t : public node_t {
public:
  void texture(sf::Texture *t, sf::Vector2i offset = {}, int size = 0,
               unsigned frames = 0) {
    texture_ = t;
    offset_ = offset;
    size_ = size;
    frames_ = frames;
    sprite_.setTexture(*texture_);
    sprite_.setTextureRect(sf::IntRect{offset_, sf::Vector2i{size_, size_}});
    sf::Vector2f origin{sprite_.getGlobalBounds().width / 2,
                        sprite_.getGlobalBounds().height / 2};
    sprite_.setOrigin(origin);
  }
  void scale(float x, float y) { sprite_.setScale(sf::Vector2f{x, y}); }
  void position(float x, float y) { sprite_.setPosition(x, y); }
  void flap_freq(unsigned millisec = 500) { flap_freq_ = millisec; }
  void update_this() override {
    if (body_) {
      if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
        body_->SetLinearVelocity(b2Vec2{0.f, 0.02f});
      sprite_.move(body_->GetPosition().x * size_,
                   body_->GetPosition().y * -size_);
    }
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - time_since_flap_)
            .count() < flap_freq_)
      return;
    time_since_flap_ = std::chrono::system_clock::now();

    offset_.x = (offset_.x + size_) % (frames_ * size_);
    sprite_.setTextureRect(sf::IntRect{offset_, sf::Vector2i{size_, size_}});
  }
  sf::FloatRect bounds() const { return sprite_.getGlobalBounds(); }

  void render(sf::RenderTarget &t, sf::RenderStates s) const override {
    t.draw(sprite_, s);
  }
  void phy_body(b2Body *body) { body_ = body; }

private:
  sf::Vector2i offset_;
  int size_;
  unsigned frames_, flap_freq_;
  sf::Texture *texture_;
  sf::Sprite sprite_;
  decltype(std::chrono::system_clock::now()) time_since_flap_;
  b2Body *body_;
};
} // namespace bb

namespace bb {
class input_g {
public:
  input_g() = delete;
  static void update_mouse_position(const sf::RenderWindow &w) {
    mouse_pos_old_ = mouse_pos_new_;
    mouse_pos_new_ = sf::Mouse::getPosition(w);
  }
  static sf::Vector2i new_mouse_position() { return mouse_pos_new_; }
  static sf::Vector2i old_mouse_position() { return mouse_pos_old_; }
  static int random(int a, int b) {
    std::uniform_int_distribution<> d(a, b);
    return d(twister_);
  }

  static void update_wall_height(float min, float max) {
    auto part1 = max - random(0, max);
    auto part2 = max - part1;
    auto *bigger = (part1 > part2) ? &part1 : &part2;
    *bigger -= random(2, 4) * min;
    top_wall_height_ = part1;
    bottom_wall_height_ = part2;
  }
  static float top_wall_height() { return top_wall_height_; }
  static float bottom_wall_height() { return bottom_wall_height_; }

private:
  static inline sf::Vector2i mouse_pos_new_;
  static inline sf::Vector2i mouse_pos_old_;
  static inline std::random_device rngdev_;
  static inline std::mt19937 twister_{rngdev_()};
  static inline float top_wall_height_;
  static inline float bottom_wall_height_;
};
} // namespace bb

namespace bb {
enum class qcmd_t { goto_main_menu, goto_game, game_over };
}

namespace bb {
struct cmdq_g {
public:
  cmdq_g() = delete;
  static void push(qcmd_t cmd) { commands.push_back(cmd); }

  static inline std::unique_ptr<node_t> menu_scene;
  static inline std::unique_ptr<node_t> game_scene;
  static inline std::unique_ptr<node_t> game_over;

  static void update() {
    while (commands.size()) {
      switch (commands.front()) {
      case qcmd_t::goto_main_menu:
        active_scene = menu_scene.get();
        break;
      case qcmd_t::goto_game:
        active_scene = game_scene.get();
        break;
      case qcmd_t::game_over:
        active_scene = game_over.get();
        break;
      }
      commands.pop_front();
    }
  }

  static inline std::list<qcmd_t> commands;
  static inline node_t *active_scene;
};
} // namespace bb

namespace bb {
class plat_t : public node_t {
public:
  void update_this() override {
    if (target_)
      if (box_.getGlobalBounds().intersects(target_->bounds()))
        cmdq_g::push(qcmd_t::game_over);
  }

  void render(sf::RenderTarget &t, sf::RenderStates s) const override {
    t.draw(box_, s);
  }
  void target(bird_t *ptr) { target_ = ptr; }
  sf::RectangleShape *box() { return &box_; }

private:
  sf::RectangleShape box_;
  bird_t *target_;
};
} // namespace bb

namespace bb {
class wall_t : public node_t {
public:
  void update_this() override {
    if (box_.getPosition().x <= teleport_boundry_) {
      box_.setPosition(spawn_point_);
      box_.setSize(sf::Vector2f{box_.getSize().x,
                                inverted_ ? input_g::bottom_wall_height()
                                          : input_g::top_wall_height()});
    }
    if (box_.getPosition().x == update_boundry_ && inverted_) {
      box_.setPosition(box_.getPosition().x, spawn_point_.y - box_.getSize().y);
    }
    if (target_)
      if (box_.getGlobalBounds().intersects(target_->bounds()))
        cmdq_g::push(qcmd_t::game_over);
    box_.move(-step_size_, 0);
  }

  void render(sf::RenderTarget &t, sf::RenderStates s) const override {
    t.draw(box_, s);
  }
  void target(bird_t *ptr) { target_ = ptr; }
  sf::RectangleShape *box() { return &box_; }

  void teleport_boundry(float x) { teleport_boundry_ = x; }
  void spawn_point(sf::Vector2f x) { spawn_point_ = x; }
  void step_size(float x) { step_size_ = x; }
  void invert(bool x) { inverted_ = x; }
  void update_boundry(float x) { update_boundry_ = x; }

private:
  sf::RectangleShape box_;
  bird_t *target_;
  sf::Vector2f spawn_point_;
  float teleport_boundry_;
  float update_boundry_;
  float step_size_;
  bool inverted_;
};
} // namespace bb

namespace bb {
class button_t : public node_t {
public:
  void update_this() override {
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
        box_.getGlobalBounds().contains(
            static_cast<sf::Vector2f>(input_g::new_mouse_position())))
      pressed_ = true;
    else
      pressed_ = false;
    if (on_click_ && pressed_)
      on_click_(this);
  }

  void render(sf::RenderTarget &t, sf::RenderStates s) const override {
    t.draw(box_, s);
    if (font_)
      t.draw(label_, s);
  }

  void on_click(std::function<void(node_t *)> cb) { on_click_ = std::move(cb); }
  void label(const std::string &str, sf::Font *f = nullptr) {
    if (f)
      font_ = f;
    label_.setFont(*font_);
    label_.setString(str);
  }

  void label_color(const sf::Color &c) { label_.setFillColor(c); }
  void label_size(unsigned size) { label_.setCharacterSize(size); }
  void label_center() {
    auto x =
        (box_.getGlobalBounds().width - label_.getGlobalBounds().width) / 2.f;
    auto y =
        (box_.getGlobalBounds().height - label_.getGlobalBounds().height) / 2.f;

    label_.setPosition(box_.getPosition() +
                       sf::Vector2f{x - box_.getOutlineThickness(),
                                    y - box_.getOutlineThickness()});
  }

  void box_size(const sf::Vector2f &size) { box_.setSize(size); }
  void box_color(const sf::Color &color) { box_.setFillColor(color); }
  void box_stroke(const sf::Color &c, unsigned t = 0) {
    box_.setOutlineColor(c);
    if (t)
      box_.setOutlineThickness(t);
  }
  void box_position(const sf::Vector2f &pos) { box_.setPosition(pos); }

private:
  bool pressed_;
  sf::Font *font_;
  sf::RectangleShape box_;
  sf::Text label_;
  std::function<void(button_t *)> on_click_;
};
} // namespace bb
