#include <array>
#include <cmath>
#include <memory>
#include <numeric>
#include <optional>

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include <vector>

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 480;

struct Vec3
{
    float x, y, z;

    [[nodiscard]] float distance(Vec3 other) const
    {
        return std::sqrt(std::pow(x - other.x, 2.0f) + std::pow(y - other.y, 2.0f));
    }

    friend Vec3 operator*(float a, const Vec3 &v)
    {
        return {a * v.x, a * v.y, a * v.z};
    }

    friend Vec3 operator/(const Vec3 &v, float a)
    {
        return {v.x / a, v.y / a, v.z / a};
    }

    friend Vec3 operator+(const Vec3 &v, const Vec3 &w)
    {
        return {v.x + w.x, v.y + w.y, v.z + w.z};
    }
};

struct Vertex
{
    Vec3 position;
    Vec3 color;
};

float edge_func(Vec3 point, Vec3 v0, Vec3 v1)
{
    return (point.x - v0.x) * (v1.y - v0.y) - (point.y - v0.y) * (v1.x - v0.x);
}

struct Triangle
{
    std::array<Vertex, 3> vertices;

    [[nodiscard]] bool is_inside(Vec3 point) const
    {
        return edge_func(point, vertices[0].position, vertices[2].position) > 0 &&
               edge_func(point, vertices[2].position, vertices[1].position) > 0 &&
               edge_func(point, vertices[1].position, vertices[0].position) > 0;
    }

    [[nodiscard]] Vec3 get_color(Vec3 point) const
    {
        std::array<float, 3> subtriangle_areas{
            0.5f * edge_func(point, vertices[1].position, vertices[2].position),
            0.5f * edge_func(point, vertices[2].position, vertices[0].position),
            0.5f * edge_func(point, vertices[0].position, vertices[1].position),
        };
        auto total_area = subtriangle_areas[0] + subtriangle_areas[1] + subtriangle_areas[2];

        // Barycentric coordinates
        auto l0 = subtriangle_areas[0] / total_area;
        auto l1 = subtriangle_areas[1] / total_area;
        auto l2 = subtriangle_areas[2] / total_area;

        Vec3 color = (l0 * vertices[0].color + l1 * vertices[1].color + l2 * vertices[2].color) /
                     (l0 + l1 + l2);

        return color;
    }
};

void do_draw(SDL_Texture *texture)
{
    static Triangle triangle{
        Vertex{Vec3{WINDOW_WIDTH / 4.0f, WINDOW_HEIGHT - 50.0f, 0.0f}, Vec3{1.0f, 0.0f, 0.0f}},
        Vertex{Vec3{WINDOW_WIDTH / 2.0f, 50.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}},
        Vertex{
            Vec3{WINDOW_WIDTH * 3.0f / 4.0f, WINDOW_HEIGHT - 50.0f, 0.0f},
            Vec3{0.0f, 0.0, 1.0f}},
    };

    void *data;
    int pitch;
    SDL_assert(SDL_LockTexture(texture, nullptr, &data, &pitch) == 0);

    auto *image_data = static_cast<std::uint8_t *>(data);
    auto channels = pitch / WINDOW_WIDTH;

    for (int j = 0; j < WINDOW_HEIGHT; ++j)
    {
        for (int i = 0; i < WINDOW_WIDTH; ++i)
        {
            auto pixel = Vec3{static_cast<float>(i), static_cast<float>(j), 0.0f};
            auto is_inside = triangle.is_inside(pixel);

            auto color = [&]() -> Vec3 {
                if (is_inside)
                {
                    return triangle.get_color(pixel);
                }
                else
                {
                    return Vec3{0.0f, 0.0f, 0.0f};
                }
            }();

            image_data[j * pitch + i * channels + 0] = static_cast<std::uint8_t>(color.x * 255.0);
            image_data[j * pitch + i * channels + 1] = static_cast<std::uint8_t>(color.y * 255.0);
            image_data[j * pitch + i * channels + 2] = static_cast<std::uint8_t>(color.z * 255.0);
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
            SDL_PIXELFORMAT_ABGR8888,
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
