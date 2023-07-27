#include <array>
#include <cmath>
#include <memory>
#include <span>
#include <vector>

#include "SDL2/SDL.h"
#include "glm/glm.hpp"

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4x4 = glm::mat4;

struct Vertex
{
    Vec4 position;
    Vec3 color;
    Vec2 uv;
};

struct Triangle
{
    std::array<Vertex, 3> vertices;

    [[nodiscard]] bool is_inside(Mat4x4 transform, Vec2 point) const;
    [[nodiscard]] Vertex interpolate(Mat4x4 transform, Vec2 point) const;
};

class Rasterizer
{
    int m_width, m_height;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> m_target;
    std::vector<std::uint8_t> m_depth_buffer;

    std::uint8_t *m_target_data{nullptr};
    int m_target_pitch{0};
    int m_target_channels{0};
    
    Mat4x4 m_viewport_transform{};
    Mat4x4 m_full_transform{};
    SDL_Surface *m_active_texture{nullptr};

  public:
    explicit Rasterizer(SDL_Renderer *renderer, int width, int height);

    void Render(SDL_Renderer *renderer);

    void BeginDraw();
    void FinishDraw();

    void ActivateTexture(SDL_Surface *new_texture);
    void ActivateModel(Mat4x4 model_matrix);

    void DrawPixel(int x, int y, int z, Vec3 color);
    void DrawTriangle(const Triangle &triangle);
    void DrawTriangleArray(std::span<Triangle> triangles);
    void DrawIndexed(std::span<Vertex> vertices, std::span<std::size_t> indices);
};
