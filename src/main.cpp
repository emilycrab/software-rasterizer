#include <array>
#include <memory>
#include <numeric>

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include <vector>

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 480;

struct Vec2
{
    float x, y;
};

bool edge_func(Vec2 pixel, Vec2 v0, Vec2 v1)
{
    return ((pixel.x - v0.x) * (v1.y - v0.y) - (pixel.y - v0.y) * (v1.x - v0.x)) > 0;
}

void do_draw(SDL_Texture *texture)
{
    static std::array<Vec2, 3> triangle{
        Vec2{WINDOW_WIDTH / 4.0f, 50.0f},
        Vec2{WINDOW_WIDTH * 3.0f / 4.0f, 50.0f},
        Vec2{WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT - 50.0f},
    };

    void *data;
    int pitch;
    SDL_LockTexture(texture, nullptr, &data, &pitch);
    SDL_assert(pitch == 4 * WINDOW_WIDTH);

    auto *image_data = static_cast<std::uint8_t *>(data);

    for (int j = 0; j < WINDOW_HEIGHT; ++j)
    {
        for (int i = 0; i < WINDOW_WIDTH; ++i)
        {
            auto pixel = Vec2{static_cast<float>(i), static_cast<float>(j)};
            auto is_inside = edge_func(pixel, triangle[0], triangle[2]) &&
                             edge_func(pixel, triangle[2], triangle[1]) &&
                             edge_func(pixel, triangle[1], triangle[0]); 

            auto color = [i,j,is_inside]() {
                if (is_inside)
                {
                    return SDL_Color{255, 255, 255, 255};
                }
                else
                {
                    return SDL_Color{0, 0, 0, 0};
                }
            }();

            image_data[(j * WINDOW_WIDTH + i) * 4 + 0] = color.r;
            image_data[(j * WINDOW_WIDTH + i) * 4 + 1] = color.g;
            image_data[(j * WINDOW_WIDTH + i) * 4 + 2] = color.b;
            image_data[(j * WINDOW_WIDTH + i) * 4 + 3] = color.a;
        }
    }

    SDL_UnlockTexture(texture);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    SDL_SetMainReady();

    SDL_assert(SDL_Init(SDL_INIT_VIDEO) == 0);

    auto window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>(
        SDL_CreateWindow(
            "Software Rasterizer",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_SHOWN
        ),
        &SDL_DestroyWindow
    );
    SDL_assert(window != nullptr);

    auto renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>(
        SDL_CreateRenderer(window.get(), -1, 0),
        &SDL_DestroyRenderer
    );
    SDL_assert(renderer != nullptr);

    auto texture = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>(
        SDL_CreateTexture(
            renderer.get(),
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            WINDOW_WIDTH,
            WINDOW_HEIGHT
        ),
        SDL_DestroyTexture
    );
    SDL_assert(texture != nullptr);

    do_draw(texture.get());
    SDL_Log("Done Drawing!");

    SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
    SDL_RenderPresent(renderer.get());

    auto should_quit = false;
    while (!should_quit)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event) == 1)
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    should_quit = true;
                    break;
            }
        }
    }

    SDL_Quit();

    return 0;
}
