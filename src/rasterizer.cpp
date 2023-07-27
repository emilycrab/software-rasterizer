#include <algorithm>

#include "glm/gtx/transform.hpp"

#include "rasterizer.hpp"

float edge_func(Vec2 point, Vec4 v0, Vec4 v1)
{
    return (point.x - v0.x) * (v1.y - v0.y) - (point.y - v0.y) * (v1.x - v0.x);
}

[[nodiscard]] bool Triangle::is_inside(Mat4x4 transform, Vec2 point) const
{
    auto v0 = transform * vertices[0].position;
    auto v1 = transform * vertices[1].position;
    auto v2 = transform * vertices[2].position;

    auto w0 = edge_func(point, v0, v2);
    auto w1 = edge_func(point, v2, v1);
    auto w2 = edge_func(point, v1, v0);

    //return (w0 > 0 && w1 > 0 && w2 > 0) || (w0 < 0 && w1 < 0 && w2 < 0);
    return (w0 < 0 && w1 < 0 && w2 < 0);
    //return (w0 > 0 && w1 > 0 && w2 > 0);
}

[[nodiscard]] Vertex Triangle::interpolate(Mat4x4 transform, Vec2 point) const
{
    auto v0 = transform * vertices[0].position;
    auto v1 = transform * vertices[1].position;
    auto v2 = transform * vertices[2].position;

    std::array<float, 3> subtriangle_areas{
        0.5f * edge_func(point, v1, v2),
        0.5f * edge_func(point, v2, v0),
        0.5f * edge_func(point, v0, v1),
    };
    auto total_area = subtriangle_areas[0] + subtriangle_areas[1] + subtriangle_areas[2];

    // Barycentric coordinates
    auto l0 = subtriangle_areas[0] / total_area;
    auto l1 = subtriangle_areas[1] / total_area;
    auto l2 = subtriangle_areas[2] / total_area;

    auto position = (l0 * v0 + l1 * v1 + l2 * v2) / (l0 + l1 + l2);

    auto color =
        (l0 * vertices[0].color + l1 * vertices[1].color + l2 * vertices[2].color) / (l0 + l1 + l2);

    auto uv = (l0 * vertices[0].uv + l1 * vertices[1].uv + l2 * vertices[2].uv) / (l0 + l1 + l2);

    return Vertex{position, color, uv};
}

Rasterizer::Rasterizer(SDL_Renderer *renderer, int width, int height)
    : m_width{width}, m_height{height},
      m_target(
          SDL_CreateTexture(
              renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height
          ),
          SDL_DestroyTexture
      ),
      m_depth_buffer(width * height, 255)
{
    SDL_assert(m_target != nullptr);

    m_viewport_transform = Mat4x4{
        Vec4{m_width / 2.0f, 0.0f, 0.0f, 0.0f},
        Vec4{0.0f, -m_height / 2.0f, 0.0f, 0.0f},
        Vec4{0.0f, 0.0f, 0.5f, 0.0f},
        Vec4{m_width / 2.0f, m_height / 2.0f, 0.5f, 1.0f},
    };
}

void Rasterizer::Render(SDL_Renderer *renderer)
{
    SDL_RenderCopy(renderer, m_target.get(), nullptr, nullptr);
}

void Rasterizer::BeginDraw()
{
    void *data = nullptr;
    if (SDL_LockTexture(m_target.get(), nullptr, &data, &m_target_pitch) != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", SDL_GetError());
        std::exit(1);
    }
    m_target_data = static_cast<std::uint8_t *>(data);
    m_target_channels = m_target_pitch / m_width;

    std::fill_n(m_target_data, m_height * m_target_pitch, 0);
    std::fill(std::begin(m_depth_buffer), std::end(m_depth_buffer), 255);
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

void Rasterizer::ActivateModel(Mat4x4 model_matrix)
{
    m_full_transform = m_viewport_transform * model_matrix;
}

void Rasterizer::DrawPixel(int x, int y, int z, Vec3 color)
{
    auto depth_buffer_idx = y * m_width + x;
    if (z < m_depth_buffer[depth_buffer_idx])
    {
        m_depth_buffer[depth_buffer_idx] = z;

        m_target_data[y * m_target_pitch + x * m_target_channels + 0] =
            static_cast<std::uint8_t>(color.x * 255.0);
        m_target_data[y * m_target_pitch + x * m_target_channels + 1] =
            static_cast<std::uint8_t>(color.y * 255.0);
        m_target_data[y * m_target_pitch + x * m_target_channels + 2] =
            static_cast<std::uint8_t>(color.z * 255.0);
    }
}

void Rasterizer::DrawTriangle(const Triangle &triangle)
{
    for (int j = 0; j < m_height; ++j)
    {
        for (int i = 0; i < m_width; ++i)
        {
            auto pixel = Vec2{static_cast<float>(i), static_cast<float>(j)};
            if (triangle.is_inside(m_full_transform, pixel))
            {
                auto interpolated = triangle.interpolate(m_full_transform, pixel);
                auto color = interpolated.color;

                if (m_active_texture != nullptr)
                {
                    auto tex_x = static_cast<int>(interpolated.uv.x * m_active_texture->w);
                    auto tex_y = static_cast<int>(interpolated.uv.y * m_active_texture->h);
                    auto idx = tex_y * m_active_texture->pitch +
                              tex_x * m_active_texture->format->BytesPerPixel;
                    auto *pixels = static_cast<std::uint8_t *>(m_active_texture->pixels);
                    color = color * Vec3{
                                        pixels[idx + 0] / 255.0f,
                                        pixels[idx + 1] / 255.0f,
                                        pixels[idx + 2] / 255.0f,
                                    };
                }

                DrawPixel(i, j, interpolated.position.z, color);
            }
        }
    }
}

void Rasterizer::DrawTriangleArray(std::span<Triangle> triangles)
{
    for (const auto &triangle : triangles)
    {
        DrawTriangle(triangle);
    }
}

void Rasterizer::DrawIndexed(std::span<Vertex> vertices, std::span<std::size_t> indices)
{
    for (std::size_t i = 0; i < indices.size(); i += 3)
    {
        Triangle triangle{vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]};
        DrawTriangle(triangle);
    }
}