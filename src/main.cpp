#include <filesystem>
#include <game.hpp>
#include <iostream>
#include <random>
#include <string>

int main(int argc, char **argv) {
  constexpr double frame_time = 1000000.0 / 60.0;
  auto gd = bb::initialize(argc, argv);
  sf::Event event{};

  while (gd.window->isOpen()) {
    while (gd.window->pollEvent(event))
      if (event.type == sf::Event::KeyPressed)
        if (event.key.code == sf::Keyboard::Escape)
          gd.window->close();

    if (bb::elapsed_time(gd.frame_begin) >= frame_time) {
      gd.frame_begin = std::chrono::steady_clock::now();
      bb::update(gd);
    }
    bb::render(gd);
  }
  std::cout << gd.score << std::endl;
  return 0;
}

namespace bb {
void render(game_t &gd) {
  gd.window->clear();
  if (gd.active_scene)
    gd.window->draw(*gd.active_scene);
  gd.window->display();
}

void spawn_fences(game_t &gd);

void update(game_t &gd) {
  static constexpr double step = 1.0 / 60.0;
  tracker_t::update(*gd.window);
  cmdsys_t::update(gd);

  if (gd.update_target == cmd_t::goto_action) {
    static bool check_pos = true;
    gd.phyworld.Step(step, 6, 2);

    gd.top_fence->drawable().move(-gd.fence_speed, 0);
    gd.bottom_fence->drawable().move(-gd.fence_speed, 0);

    auto topf_bb = gd.top_fence->drawable().getGlobalBounds();
    auto bottomf_bb = gd.bottom_fence->drawable().getGlobalBounds();
    auto topw_bb = gd.top_wall->drawable().getGlobalBounds();
    auto bottomw_bb = gd.bottom_wall->drawable().getGlobalBounds();
    auto bird_bb = gd.player->drawable().getGlobalBounds();

    if (bird_bb.intersects(topf_bb) || bird_bb.intersects(bottomf_bb) ||
        bird_bb.intersects(topw_bb) || bird_bb.intersects(bottomw_bb)) {
      cmdsys_t::push(cmd_t::goto_game_over);
    }

    if (check_pos) {
      if (bird_bb.left - bird_bb.width / 2.f > topf_bb.left) {
        check_pos = false;
        ++gd.score;
      }
    }
    if (bird_bb.left - bird_bb.width / 2.f < topf_bb.left)
      check_pos = true;
    if (topf_bb.left < -topf_bb.width)
      spawn_fences(gd);
  }

  if (gd.active_scene)
    gd.active_scene->update(gd);
}
} // namespace bb

namespace bb {
std::unique_ptr<sf::RenderWindow> make_window(const game_t &gd) {
  std::unique_ptr<sf::RenderWindow> window{new sf::RenderWindow{
      sf::VideoMode{gd.window_width, gd.window_height}, "bouncy bird",
      sf::Style::Titlebar | sf::Style::Close}};
  auto res = sf::VideoMode::getDesktopMode();
  window->setPosition(
      sf::Vector2i{(int)res.width / 2 - (int)gd.window_width / 2,
                   (int)res.height / 2 - (int)gd.window_height / 2});
  window->setVerticalSyncEnabled(false);
  return window;
}
} // namespace bb

namespace bb {
sf::Vector2f to_world_coords(const game_t &gd, sf::Vector2f bc) {
  bc *= (float)gd.pix_per_meter;
  bc.y *= -1.f;
  bc +=
      sf::Vector2f{(float)gd.window_width / 2.f, (float)gd.window_height / 2.f};
  return bc;
}
} // namespace bb

namespace bb {
void initialize_resources(game_t &);
void initialize_working_directory(std::filesystem::path);
void initialize_window(int, char **, game_t &);

game_t initialize(int argc, char **argv) {
  initialize_working_directory(argv[0]);
  game_t data{.bird_texture = {},
              .main_font = {},
              .window_width = 640,
              .window_height = 480,
              .bird_tex_size = {},
              .bird_frame_count = {},
              .bird_flap_force = 10000,
              .bird_density = 1.f,
              .bird_body = {},
              .bird_tex_offset = {},
              .pix_per_meter = 25,
              .wall_thickness = 0.02,
              .phyworld = b2World{b2Vec2{0.f, -9.89f}},
              .window = {},
              .frame_begin = std::chrono::steady_clock::now(),
              .main_menu_scene = {},
              .action_scene = {},
              .game_over_scene = {},
              .active_scene = {},
              .top_wall = {},
              .bottom_wall = {},
              .player = {},
              .top_fence = {},
              .bottom_fence = {},
              .lead_color = sf::Color::White,
              .follow_color = sf::Color::Magenta,
              .fence_speed = 2.f,
              .score = 0,
              .update_target = {},
              .button_width = 0.5,
              .button_height = 0.1,
              .button_outline = 5.f,
              .button_padding = 15.f,
              .text_size = 0.7f};

  initialize_window(argc, argv, data);
  initialize_resources(data);
  cmdsys_t::push(cmd_t::goto_main_menu);
  return data;
}

void initialize_window(int argc, char **argv, game_t &data) {
  try {
    if (argc > 2) {
      data.window_width = std::stoi(argv[1]);
      data.window_height = std::stoi(argv[2]);
      if (data.window_width < 640 || data.window_height < 480) {
        data.window_width = 640, data.window_height = 480;
      }
    }
  } catch (const std::exception &) {
    data.window_width = 640, data.window_height = 480;
  }
  data.window = make_window(data);
}

void initialize_game_over_scene(game_t &gd) {
  gd.game_over_scene = std::make_unique<node_i>();
  float button_w = gd.window_width * gd.button_width;
  float button_h = gd.window_height * gd.button_height;
  sf::Vector2f size{button_w, button_h};

  auto make_button = [&](const std::string &ll, unsigned i) {
    auto button = new button_t{};
    gd.game_over_scene->add(std::unique_ptr<node_i>{button});
    button->label().setFont(gd.main_font);
    button->label().setCharacterSize(gd.text_size * button_h);
    button->label().setString(ll);
    button->label().setFillColor(gd.lead_color);
    sf::Vector2f pos{gd.window_width / 2.f - button_w / 2.f,
                     (float)i * (button_h + gd.button_padding)};
    button->box().setPosition(pos);
    button->box().setSize({button_w, button_h});
    button->box().setFillColor(sf::Color::Black);
    button->box().setOutlineColor(gd.follow_color);
    button->box().setOutlineThickness(gd.button_outline);
    auto bb = button->box().getGlobalBounds();
    auto &l = button->label();
    l.setPosition(sf::Vector2f{
        bb.left +
            (bb.width - gd.button_outline - l.getGlobalBounds().width) / 2.f,
        bb.top +
            (bb.height - 2 * gd.button_outline - l.getGlobalBounds().height) /
                2.f});
    return button;
  };

  make_button("Game over!", 1);
  make_button("Score: " + std::to_string(gd.score), 2);
  auto pa = make_button("Play again", 3);
  pa->onclick([](game_t &, button_t *) { cmdsys_t::push(cmd_t::goto_action); });
  auto ex = make_button("Exit", 4);
  ex->onclick([](game_t &gd, button_t *) { gd.window->close(); });
}

void initialize_main_menu_scene(game_t &gd) {
  gd.main_menu_scene = std::make_unique<node_i>();
  float play_w = gd.window_width * gd.button_width;
  float play_h = gd.window_height * gd.button_height;
  sf::Vector2f pos{gd.window_width / 2.f - play_w / 2.f,
                   gd.window_height / 2.f - play_h / 2.f};

  auto play = new button_t{};
  gd.main_menu_scene->add(std::unique_ptr<node_i>{play});

  play->box().setOutlineThickness(gd.button_outline);
  play->box().setOutlineColor(gd.follow_color);
  play->box().setFillColor(sf::Color::Black);
  play->box().setPosition(pos);
  play->box().setSize({play_w, play_h});

  play->label().setFont(gd.main_font);
  play->label().setCharacterSize(gd.text_size * play_h);
  play->label().setString("Play");
  play->label().setFillColor(gd.lead_color);
  auto bb = play->box().getGlobalBounds();
  auto &l = play->label();
  l.setPosition(sf::Vector2f{
      bb.left +
          (bb.width - gd.button_outline - l.getGlobalBounds().width) / 2.f,
      bb.top +
          (bb.height - 2 * gd.button_outline - l.getGlobalBounds().height) /
              2.f});
  play->onclick(
      [](game_t &, button_t *) { cmdsys_t::push(cmd_t::goto_action); });
}

void initialize_bird(game_t &gd);
void initialize_fences(game_t &gd);

void initialize_action_scene(game_t &gd) {
  gd.action_scene = std::make_unique<node_i>();
  gd.score = 0;

  auto make_and_attach_wall = [&]() {
    auto wall = new wall_t{};
    gd.action_scene->add(std::unique_ptr<node_i>{wall});
    return wall;
  };

  auto make_wall_drawable = [&]() {
    typename wall_t::drawable_t body{};
    float wall_w = gd.window_width;
    float wall_h = gd.wall_thickness * gd.window_height;
    body.setSize(sf::Vector2f{wall_w, wall_h});
    body.setFillColor(gd.follow_color);
    return body;
  };

  gd.top_wall = make_and_attach_wall();
  gd.top_wall->drawable() = make_wall_drawable();

  gd.bottom_wall = make_and_attach_wall();
  auto &bwall = gd.bottom_wall->drawable();
  bwall = make_wall_drawable();
  bwall.setPosition({0, gd.window_height - bwall.getSize().y});

  initialize_bird(gd);
  initialize_fences(gd);
}

void spawn_fences(game_t &gd) {
  static const float fence_w = gd.wall_thickness * gd.window_height;
  static const float spawn_x = gd.window_width + 10.f;
  static std::random_device dev{};
  static std::mt19937 twister{dev()};

  float blen = gd.player->drawable().getGlobalBounds().width;
  float mult = gd.window_height < 1080 ? 2.f : 1.5f;
  float avail_height = gd.window_height - mult * blen;
  std::uniform_int_distribution<int> dist(0, avail_height);

  float top_fence_h = dist(twister);
  float bottom_fence_h = avail_height - top_fence_h;

  gd.top_fence->drawable().setPosition({spawn_x, 0});
  gd.bottom_fence->drawable().setPosition(
      {spawn_x, gd.window_height - bottom_fence_h});

  gd.top_fence->drawable().setFillColor(gd.follow_color);
  gd.bottom_fence->drawable().setFillColor(gd.follow_color);

  gd.top_fence->drawable().setSize({fence_w, top_fence_h});
  gd.bottom_fence->drawable().setSize({fence_w, bottom_fence_h});
}

void initialize_fences(game_t &gd) {
  auto make_and_attach_fence = [&]() {
    auto fence = new fence_t{};
    gd.action_scene->add(std::unique_ptr<node_i>{fence});
    return fence;
  };
  gd.top_fence = make_and_attach_fence();
  gd.bottom_fence = make_and_attach_fence();
  spawn_fences(gd);
  if (gd.window_height < 1080) {
    gd.fence_speed = 1.f;
  }
}

void initialize_bird(game_t &gd) {
  if (gd.bird_body)
    gd.phyworld.DestroyBody(gd.bird_body);
  gd.player = new bird_t{};
  gd.action_scene->add(std::unique_ptr<node_i>{gd.player});
  gd.player->texture(gd.bird_texture);
  gd.player->animation(200, gd.bird_frame_count, gd.bird_tex_size,
                       gd.bird_tex_offset);
  gd.player->drawable().setPosition(gd.window_width / 2, gd.window_height / 2);
  gd.player->center();
  gd.player->drawable().setScale(sf::Vector2f{-1, 1});

  unsigned var = 40;
  if (gd.window_height < 1080) {
    constexpr float mult = 0.5f;
    gd.player->drawable().scale(mult, mult);
    gd.bird_flap_force *= mult * mult;
    gd.bird_density *= mult;
    var *= mult;
  }
  gd.player->flap_force(gd.bird_flap_force);

  b2BodyDef body_def;
  body_def.position.Set(0.f, 0.f);
  body_def.type = b2_dynamicBody;
  gd.bird_body = gd.phyworld.CreateBody(&body_def);
  b2PolygonShape box;
  auto bb = gd.player->drawable().getGlobalBounds();
  box.SetAsBox(bb.width / gd.pix_per_meter, bb.height / gd.pix_per_meter);
  gd.bird_body->CreateFixture(&box, gd.bird_density);
  gd.player->body(gd.bird_body);
}

void initialize_resources(game_t &gd) {
  bool result = gd.main_font.loadFromFile("./font/Exo-Regular.ttf");
  if (!result)
    throw std::runtime_error{"Failed to load the main game font!"};

  result = gd.bird_texture.loadFromFile("./img/BirdSprite.png");
  if (!result)
    throw std::runtime_error{"Failed to load the bird sprite sheet!"};
  gd.bird_tex_size = 160;
  gd.bird_tex_offset = sf::Vector2f{0, 160};
  gd.bird_frame_count = 8;
}

void initialize_working_directory(std::filesystem::path exec) {
  std::filesystem::current_path(std::filesystem::absolute(exec).parent_path());
}
} // namespace bb