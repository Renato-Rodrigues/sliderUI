// src/ui/input_sdl.cpp
#include "ui/renderer.h"
#include <SDL/SDL.h>

namespace ui {

Input poll_input() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                return Input::B;
            case SDL_KEYDOWN: {
                SDLKey k = ev.key.keysym.sym;
                switch (k) {
                    case SDLK_UP:    return Input::UP;
                    case SDLK_DOWN:  return Input::DOWN;
                    case SDLK_LEFT:  return Input::LEFT;
                    case SDLK_RIGHT: return Input::RIGHT;
                    case SDLK_z:     return Input::A;
                    case SDLK_x:     return Input::B;
                    case SDLK_a:     return Input::X;
                    case SDLK_s:     return Input::Y;
                    case SDLK_RETURN: return Input::START;
                    case SDLK_KP_ENTER: return Input::START;
                    case SDLK_TAB:    return Input::SELECT;
                    case SDLK_m:      return Input::MENU;
                    case SDLK_ESCAPE: return Input::B;
                    default: break;
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT) return Input::A;
                break;
            default:
                break;
        }
    }
    return Input::NONE;
}

} // namespace ui
