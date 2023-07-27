#include <array>
#include <cmath>
#include <memory>
#include <span>

#include "SDL2/SDL.h"

struct Vec3
{
    float x, y, z;

    [[nodiscard]] float distance(Vec3 other) const
    {
        return std::sqrt(std::pow(x - other.x, 2.0f) + std::pow(y - other.y, 2.0f));
    }

    friend Vec3 operator*(const Vec3 &v, const Vec3 &w)
    {
        return {v.x * w.x, v.y * w.y, v.z * w.z};
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
    Vec3 uv;
};

struct Triangle
{
    std::array<Vertex, 3> vertices;

    [[nodiscard]] bool is_inside(Vec3 point) const;
    [[nodiscard]] Vertex interpolate(Vec3 point) const;
};

class Rasterizer
{
    int m_width, m_height;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> m_target;

    std::uint8_t *m_target_data{nullptr};
    int m_target_pitch{0};
    int m_target_channels{0};

    SDL_Surface *m_active_texture{nullptr};

  public:
    explicit Rasterizer(SDL_Renderer *renderer, int width, int height);

    void Render(SDL_Renderer *renderer);

    void BeginDraw();
    void FinishDraw();

    void ActivateTexture(SDL_Surface *new_texture);

    void DrawPixel(int x, int y, Vec3 color);
    void DrawTriangle(const Triangle &triangle);
    void DrawTriangleArray(std::span<Triangle> triangles);
};
