#include <algorithm>

#include "rasterizer.hpp"

float edge_func(Vec3 point, Vec3 v0, Vec3 v1)
{
    return (point.x - v0.x) * (v1.y - v0.y) - (point.y - v0.y) * (v1.x - v0.x);
}

[[nodiscard]] bool Triangle::is_inside(Vec3 point) const
{
    return edge_func(point, vertices[0].position, vertices[2].position) > 0 &&
           edge_func(point, vertices[2].position, vertices[1].position) > 0 &&
           edge_func(point, vertices[1].position, vertices[0].position) > 0;
}

[[nodiscard]] Vertex Triangle::interpolate(Vec3 point) const
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

    Vec3 color =
        (l0 * vertices[0].color + l1 * vertices[1].color + l2 * vertices[2].color) / (l0 + l1 + l2);

    Vec3 uv = (l0 * vertices[0].uv + l1 * vertices[1].uv + l2 * vertices[2].uv) / (l0 + l1 + l2);

    return Vertex{point, color, uv};
}

Rasterizer::Rasterizer(SDL_Renderer *renderer, int width, int height)
    : m_width{width}, m_height{height},
      m_target(
          SDL_CreateTexture(
              renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height
          ),
          SDL_DestroyTexture
      )
{
    SDL_assert(m_target != nullptr);
}

void Rasterizer::Render(SDL_Renderer *renderer)
{
    SDL_RenderCopy(renderer, m_target.get(), nullptr, nullptr);
}

void Rasterizer::BeginDraw()
{
    void *data;
    SDL_assert(SDL_LockTexture(m_target.get(), nullptr, &data, &m_target_pitch) == 0);
    m_target_data = static_cast<std::uint8_t *>(data);
    m_target_channels = m_target_pitch / m_width;

    std::fill_n(m_target_data, m_height * m_target_pitch, 0);
}

void Rasterizer::FinishDraw()
{
    SDL_UnlockTexture(m_target.get());
    m_target_data = nullptr;
    m_target_pitch = 0;
    m_target_channels = 0;
}

void Rasterizer::ActivateTexture(SDL_Surface *new_texture)
{
    m_active_texture = new_texture;
}

void Rasterizer::DrawPixel(int x, int y, Vec3 color)
{
    m_target_data[y * m_target_pitch + x * m_target_channels + 0] =
        static_cast<std::uint8_t>(color.x * 255.0);
    m_target_data[y * m_target_pitch + x * m_target_channels + 1] =
        static_cast<std::uint8_t>(color.y * 255.0);
    m_target_data[y * m_target_pitch + x * m_target_channels + 2] =
        static_cast<std::uint8_t>(color.z * 255.0);
}

void Rasterizer::DrawTriangle(const Triangle &triangle)
{
    for (int j = 0; j < m_height; ++j)
    {
        for (int i = 0; i < m_width; ++i)
        {
            auto pixel = Vec3{static_cast<float>(i), static_cast<float>(j), 0.0f};
            if (triangle.is_inside(pixel))
            {
                auto interpolated = triangle.interpolate(pixel);
                auto color = interpolated.color;

                if (m_active_texture != nullptr)
                {
                    int tex_x = interpolated.uv.x * m_active_texture->w;
                    int tex_y = interpolated.uv.y * m_active_texture->h;
                    int idx = tex_y * m_active_texture->pitch +
                              tex_x * m_active_texture->format->BytesPerPixel;
                    auto *pixels = static_cast<std::uint8_t *>(m_active_texture->pixels);
                    color = color * Vec3{
                                        pixels[idx + 0] / 255.0f,
                                        pixels[idx + 1] / 255.0f,
                                        pixels[idx + 2] / 255.0f,
                                    };
                }

                DrawPixel(i, j, color);
            }
        }
    }
}

void Rasterizer::DrawTriangleArray(std::span<Triangle> triangles)
{
    for (const auto &triangle : triangles) {
        DrawTriangle(triangle);
    }
}
