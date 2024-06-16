#pragma once
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <vector>

namespace bb {
using time_format_t = std::chrono::time_point<std::chrono::steady_clock>;
inline double elapsed_time(const time_format_t &old) {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(now - old)
      .count();
}
} // namespace bb

namespace bb {
enum class cmd_t { goto_main_menu, goto_action, goto_game_over };

enum class node_state_t : unsigned {
  visible = 1 << 0,
  hidden = ~visible,
  active = 1 << 1,
  passive = ~active,
  initial = visible | active
};
node_state_t operator|(node_state_t a, node_state_t b) {
  return static_cast<node_state_t>(static_cast<unsigned>(a) |
                                   static_cast<unsigned>(b));
}
node_state_t operator&(node_state_t a, node_state_t b) {
  return static_cast<node_state_t>(static_cast<unsigned>(a) &
                                   static_cast<unsigned>(b));
}

class node_i;

template <typename T> class basic_node_t;

class bird_t;

using wall_t = basic_node_t<sf::RectangleShape>;
using fence_t = basic_node_t<sf::RectangleShape>;

struct game_t {
  sf::Texture bird_texture;
  sf::Font main_font;

  unsigned window_width;
  unsigned window_height;

  unsigned bird_tex_size;
  unsigned bird_frame_count;
  unsigned bird_flap_force;
  float bird_density;
  b2Body *bird_body;
  sf::Vector2f bird_tex_offset;

  unsigned pix_per_meter;
  float wall_thickness;

  b2World phyworld;
  std::unique_ptr<sf::RenderWindow> window;
  time_format_t frame_begin;

  std::unique_ptr<node_i> main_menu_scene;
  std::unique_ptr<node_i> action_scene;
  std::unique_ptr<node_i> game_over_scene;

  node_i *active_scene;

  wall_t *top_wall;
  wall_t *bottom_wall;
  bird_t *player;
  fence_t *top_fence;
  fence_t *bottom_fence;

  sf::Color lead_color;
  sf::Color follow_color;

  float fence_speed;
  std::size_t score;

  cmd_t update_target;
  float button_width;
  float button_height;
  float button_outline;
  float button_padding;

  float text_size;
};

class node_i : public sf::Drawable {
public:
  node_i() : state_{node_state_t::initial} {}
  virtual ~node_i() = default;
  node_i(const node_i &) = delete;
  node_i &operator=(const node_i &) = delete;
  node_i(node_i &&) = default;
  node_i &operator=(node_i &&) = default;

  void draw(sf::RenderTarget &t, sf::RenderStates s) const override {
    if (this->visible())
      this->render(t, s);
    for (const auto &c : nodes_)
      c->draw(t, s);
  }
  virtual void render(sf::RenderTarget &, sf::RenderStates) const {}

  void update(game_t &g) {
    if (this->active())
      this->refresh(g);
    for (const auto &c : nodes_)
      c->update(g);
  }
  virtual void refresh(game_t &) {}

  void visible(bool v) {
    v ? state_ = state_ | node_state_t::visible
      : state_ = state_ & node_state_t::hidden;
  }
  bool visible() const { return (bool)(state_ & node_state_t::visible); }

  void active(bool v) {
    v ? state_ = state_ | node_state_t::active
      : state_ = state_ & node_state_t::passive;
  }
  bool active() const { return (bool)(state_ & node_state_t::active); }

  void add(std::unique_ptr<node_i> n) { nodes_.push_back(std::move(n)); }

private:
  node_state_t state_;
  std::vector<std::unique_ptr<node_i>> nodes_;
};

template <typename T> class basic_node_t : public node_i {
public:
  using drawable_t = T;
  void render(sf::RenderTarget &t, sf::RenderStates s) const override {
    t.draw(drawable_, s);
  }
  T &drawable() { return drawable_; }

private:
  T drawable_;
};

class animated_node_t : public basic_node_t<sf::RectangleShape> {
public:
  animated_node_t()
      : texture_set_{false}, last_update_{std::chrono::steady_clock::now()} {}

  void refresh(game_t &) override {
    if (texture_set_) {
      elapsed_ += elapsed_time(last_update_);
      if (elapsed_ >= next_frame_in_) {
        last_update_ = std::chrono::steady_clock::now();
        elapsed_ = 0;
        this->next_frame();
      }
    }
  }

  void texture(sf::Texture &t) {
    this->drawable().setTexture(&t);
    texture_set_ = true;
  }

  void animation(std::size_t freq, std::size_t count, std::size_t size,
                 sf::Vector2f offset = {}) {
    next_frame_in_ = freq * 1000; // from milli to micro
    frame_count_ = count;
    frame_size_ = size;
    this->drawable().setSize(
        sf::Vector2f{(float)frame_size_, (float)frame_size_});
    auto fr = this->drawable().getTextureRect();
    fr.height = fr.width = frame_size_;
    fr.left = offset.x, fr.top = offset.y;
    this->drawable().setTextureRect(fr);
  }

  void center() {
    auto bb = this->drawable().getGlobalBounds();
    this->drawable().setOrigin(sf::Vector2f{bb.width / 2, bb.height / 2});
  }

  void next_frame() {
    auto fr = this->drawable().getTextureRect();
    fr.left = (fr.left + frame_size_) % (frame_count_ * frame_size_);
    this->drawable().setTextureRect(fr);
  }

private:
  std::size_t next_frame_in_; // microsec
  std::size_t frame_count_;
  std::size_t frame_size_;
  std::size_t elapsed_; // microsec
  bool texture_set_;
  time_format_t last_update_;
};

sf::Vector2f to_world_coords(const game_t &gd, sf::Vector2f bc);

class bird_t : public animated_node_t {
public:
  void body(b2Body *ptr) { body_ = ptr; }
  b2Body *body() { return body_; }
  void refresh(game_t &gd) override {
    animated_node_t::refresh(gd);
    auto &d = this->drawable();
    if (body_) {
      sf::Vector2f pos{body_->GetPosition().x, body_->GetPosition().y};
      d.setPosition(to_world_coords(gd, std::move(pos)));
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) ||
        sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
      body_->ApplyForce(b2Vec2{0, flap_force_}, body_->GetPosition(), true);
    }
  }
  void flap_force(float x) { flap_force_ = x; }

private:
  b2Body *body_;
  float flap_force_;
};

class tracker_t {
public:
  tracker_t() = delete;
  static void update(const sf::RenderWindow &win) {
    auto pos = sf::Mouse::getPosition(win);
    last_mouse_pos_ = sf::Vector2f{(float)pos.x, (float)pos.y};
  }
  static sf::Vector2f last_mouse_pos() { return last_mouse_pos_; }

private:
  static inline sf::Vector2f last_mouse_pos_;
};
} // namespace bb

namespace bb {
void initialize_main_menu_scene(game_t &);
void initialize_action_scene(game_t &);
void initialize_game_over_scene(game_t &);

class cmdsys_t {
public:
  cmdsys_t() = delete;
  static void push(cmd_t cmd) { cmds_.push_back(cmd); }
  static void update(game_t &gd) {
    while (cmds_.size()) {
      switch (cmds_.front()) {
      case cmd_t::goto_main_menu:
        gd.update_target = cmd_t::goto_main_menu;
        initialize_main_menu_scene(gd);
        gd.active_scene = gd.main_menu_scene.get();
        break;
      case cmd_t::goto_action:
        gd.update_target = cmd_t::goto_action;
        initialize_action_scene(gd);
        gd.active_scene = gd.action_scene.get();
        break;
      case cmd_t::goto_game_over:
        gd.update_target = cmd_t::goto_game_over;
        initialize_game_over_scene(gd);
        gd.active_scene = gd.game_over_scene.get();
        break;
      }
      cmds_.pop_front();
    }
  }

private:
  static inline std::list<cmd_t> cmds_;
};

game_t initialize(int argc, char **argv);

std::unique_ptr<sf::RenderWindow> make_window(const game_t &gd);

void render(game_t &gd);
void update(game_t &gd);

class button_t : public node_i {
public:
  void onclick(std::function<void(game_t &, button_t *)> f) { onclick_ = f; }
  sf::RectangleShape &box() { return box_; }
  sf::Text &label() { return label_; }
  void refresh(game_t &gd) override {
    if (box_.getGlobalBounds().contains(tracker_t::last_mouse_pos()))
      if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
        if (onclick_)
          onclick_(gd, this);
  }
  void render(sf::RenderTarget &t, sf::RenderStates s) const override {
    t.draw(box_, s);
    t.draw(label_, s);
  }

private:
  std::function<void(game_t &, button_t *)> onclick_;
  sf::RectangleShape box_;
  sf::Text label_;
};
} // namespace bb