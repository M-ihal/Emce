#pragma once

class Game;
class Window;

namespace Renderer {
    void init(Window &window);
    void render_world(Game &game, Window &window);
    void render_ui(Game &game, Window &window);
}
