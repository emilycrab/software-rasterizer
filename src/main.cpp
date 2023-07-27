#include <array>
#include <cmath>
#include <memory>
#include <numeric>
#include <optional>
#include <vector>

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL_image.h"

#include "rasterizer.hpp"

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 480;

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

    auto crate_img = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>(
        IMG_Load(R"(.\assets\wood_crate.jpg)"),
        SDL_FreeSurface
    );
    SDL_assert(crate_img != nullptr);

    float x = 100.0f;
    float y = 100.0f;
    float s = 300.0f;

    std::array<Triangle, 2> triangles{
        Triangle{
            // top-left
            Vertex{
                Vec3{x, y, 0.0f},
                Vec3{0.0f, 1.0f, 1.0f},
                Vec3{0.0f, 1.0f, 0.0f},
            },
            // bottom-right
            Vertex{
                Vec3{x + s, y + s, 0.0f},
                Vec3{1.0f, 0.0, 1.0f},
                Vec3{1.0f, 0.0f, 0.0f},
            },
            // bottom-left
            Vertex{
                Vec3{x, y + s, 0.0f},
                Vec3{1.0f, 1.0f, 0.0f},
                Vec3{0.0f, 0.0f, 0.0f},
            },
        },
        Triangle{
            // top-left
            Vertex{
                Vec3{x, y, 0.0f},
                Vec3{0.0f, 1.0f, 1.0f},
                Vec3{0.0f, 1.0f, 0.0f},
            },
            // top-right
            Vertex{
                Vec3{x + s, y, 0.0f},
                Vec3{1.0f, 1.0f, 1.0f},
                Vec3{1.0f, 1.0f, 0.0f},
            },
            // bottom-right
            Vertex{
                Vec3{x + s, y + s, 0.0f},
                Vec3{1.0f, 0.0, 1.0f},
                Vec3{1.0f, 0.0f, 0.0f},
            },
        },
    };

    Rasterizer rasterizer(renderer.get(), WINDOW_WIDTH, WINDOW_HEIGHT);

    rasterizer.BeginDraw();
    rasterizer.ActivateTexture(crate_img.get());
    rasterizer.DrawTriangleArray(triangles);
    rasterizer.FinishDraw();
    SDL_Log("Done Drawing!");

    rasterizer.Render(renderer.get());
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
