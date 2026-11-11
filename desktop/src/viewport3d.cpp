#include "viewport3d.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "gpu_shaders/cube.frag.dxil.h"
#include "gpu_shaders/cube.frag.msl.h"
#include "gpu_shaders/cube.frag.spv.h"
#include "gpu_shaders/cube.vert.dxil.h"
#include "gpu_shaders/cube.vert.msl.h"
#include "gpu_shaders/cube.vert.spv.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

namespace {

constexpr float kPi = 3.14159265358979323846F;

struct VertexData {
    float x, y, z;
    float red, green, blue;
};

const VertexData kCubeVertices[] = {
    {-0.5F, 0.5F, -0.5F, 1.0F, 0.0F, 0.0F},  {0.5F, -0.5F, -0.5F, 0.0F, 0.0F, 1.0F},
    {-0.5F, -0.5F, -0.5F, 0.0F, 1.0F, 0.0F}, {-0.5F, 0.5F, -0.5F, 1.0F, 0.0F, 0.0F},
    {0.5F, 0.5F, -0.5F, 1.0F, 1.0F, 0.0F},   {0.5F, -0.5F, -0.5F, 0.0F, 0.0F, 1.0F},
    {-0.5F, 0.5F, 0.5F, 1.0F, 1.0F, 1.0F},   {-0.5F, -0.5F, -0.5F, 0.0F, 1.0F, 0.0F},
    {-0.5F, -0.5F, 0.5F, 0.0F, 1.0F, 1.0F},  {-0.5F, 0.5F, 0.5F, 1.0F, 1.0F, 1.0F},
    {-0.5F, 0.5F, -0.5F, 1.0F, 0.0F, 0.0F},  {-0.5F, -0.5F, -0.5F, 0.0F, 1.0F, 0.0F},
    {-0.5F, 0.5F, 0.5F, 1.0F, 1.0F, 1.0F},   {0.5F, 0.5F, -0.5F, 1.0F, 1.0F, 0.0F},
    {-0.5F, 0.5F, -0.5F, 1.0F, 0.0F, 0.0F},  {-0.5F, 0.5F, 0.5F, 1.0F, 1.0F, 1.0F},
    {0.5F, 0.5F, 0.5F, 0.0F, 0.0F, 0.0F},    {0.5F, 0.5F, -0.5F, 1.0F, 1.0F, 0.0F},
    {0.5F, 0.5F, -0.5F, 1.0F, 1.0F, 0.0F},   {0.5F, -0.5F, 0.5F, 1.0F, 0.0F, 1.0F},
    {0.5F, -0.5F, -0.5F, 0.0F, 0.0F, 1.0F},  {0.5F, 0.5F, -0.5F, 1.0F, 1.0F, 0.0F},
    {0.5F, 0.5F, 0.5F, 0.0F, 0.0F, 0.0F},    {0.5F, -0.5F, 0.5F, 1.0F, 0.0F, 1.0F},
    {0.5F, 0.5F, 0.5F, 0.0F, 0.0F, 0.0F},    {-0.5F, -0.5F, 0.5F, 0.0F, 1.0F, 1.0F},
    {0.5F, -0.5F, 0.5F, 1.0F, 0.0F, 1.0F},   {0.5F, 0.5F, 0.5F, 0.0F, 0.0F, 0.0F},
    {-0.5F, 0.5F, 0.5F, 1.0F, 1.0F, 1.0F},   {-0.5F, -0.5F, 0.5F, 0.0F, 1.0F, 1.0F},
    {-0.5F, -0.5F, -0.5F, 0.0F, 1.0F, 0.0F}, {0.5F, -0.5F, 0.5F, 1.0F, 0.0F, 1.0F},
    {-0.5F, -0.5F, 0.5F, 0.0F, 1.0F, 1.0F},  {-0.5F, -0.5F, -0.5F, 0.0F, 1.0F, 0.0F},
    {0.5F, -0.5F, -0.5F, 0.0F, 0.0F, 1.0F},  {0.5F, -0.5F, 0.5F, 1.0F, 0.0F, 1.0F},
};

void Identity(float* m) {
    std::memset(m, 0, sizeof(float) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.0F;
}

void CopyMatrix(const float* src, float* dst) {
    std::memcpy(dst, src, sizeof(float) * 16);
}

void Perspective(float fovy_deg, float aspect, float znear, float zfar, float* r) {
    const float f = 1.0F / std::tan((fovy_deg / 180.0F) * kPi * 0.5F);
    Identity(r);
    r[0] = f / aspect;
    r[5] = f;
    r[10] = (znear + zfar) / (znear - zfar);
    r[11] = -1.0F;
    r[14] = (2.0F * znear * zfar) / (znear - zfar);
    r[15] = 0.0F;
}

void Multiply(const float* lhs, const float* rhs, float* out) {
    float tmp[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            tmp[j * 4 + i] = 0.0F;
            for (int k = 0; k < 4; ++k) {
                tmp[j * 4 + i] += lhs[k * 4 + i] * rhs[j * 4 + k];
            }
        }
    }
    std::memcpy(out, tmp, sizeof(tmp));
}

void Normalize3(float* v) {
    const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 1.0e-8F) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

void Cross3(const float* a, const float* b, float* out) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

float Dot3(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void LookAt(const float* eye, const float* center, const float* up_in, float* out) {
    float f[3] = {center[0] - eye[0], center[1] - eye[1], center[2] - eye[2]};
    Normalize3(f);
    float up[3] = {up_in[0], up_in[1], up_in[2]};
    float s[3];
    Cross3(f, up, s);
    if (s[0] * s[0] + s[1] * s[1] + s[2] * s[2] < 1.0e-8F) {
        const float alt_up[3] = {0.0F, 0.0F, 1.0F};
        Cross3(f, alt_up, s);
    }
    Normalize3(s);
    float u[3];
    Cross3(s, f, u);
    Identity(out);
    out[0] = s[0];
    out[4] = s[1];
    out[8] = s[2];
    out[1] = u[0];
    out[5] = u[1];
    out[9] = u[2];
    out[2] = -f[0];
    out[6] = -f[1];
    out[10] = -f[2];
    out[12] = -Dot3(s, eye);
    out[13] = -Dot3(u, eye);
    out[14] = Dot3(f, eye);
}

void RotationX(float angle_deg, float* out) {
    const float radians = angle_deg * kPi / 180.0F;
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    Identity(out);
    out[5] = c;
    out[6] = s;
    out[9] = -s;
    out[10] = c;
}

void TransformPoint(const float* m, float x, float y, float z, float* out) {
    out[0] = m[0] * x + m[4] * y + m[8] * z + m[12];
    out[1] = m[1] * x + m[5] * y + m[9] * z + m[13];
    out[2] = m[2] * x + m[6] * y + m[10] * z + m[14];
}

void TransformDirection(const float* m, float x, float y, float z, float* out) {
    out[0] = m[0] * x + m[4] * y + m[8] * z;
    out[1] = m[1] * x + m[5] * y + m[9] * z;
    out[2] = m[2] * x + m[6] * y + m[10] * z;
}

void GetNodeLocalMatrix(const cgltf_node* node, float* out) {
    if (node->has_matrix) {
        std::memcpy(out, node->matrix, sizeof(float) * 16);
        return;
    }
    float t[16];
    float r[16];
    float s[16];
    float tr[16];
    Identity(t);
    Identity(r);
    Identity(s);
    if (node->has_translation) {
        t[12] = node->translation[0];
        t[13] = node->translation[1];
        t[14] = node->translation[2];
    }
    if (node->has_rotation) {
        const float qx = node->rotation[0];
        const float qy = node->rotation[1];
        const float qz = node->rotation[2];
        const float qw = node->rotation[3];
        const float x2 = qx + qx;
        const float y2 = qy + qy;
        const float z2 = qz + qz;
        Identity(r);
        r[0] = 1.0F - (qy * y2 + qz * z2);
        r[1] = qx * y2 + qw * z2;
        r[2] = qx * z2 - qw * y2;
        r[4] = qx * y2 - qw * z2;
        r[5] = 1.0F - (qx * x2 + qz * z2);
        r[6] = qy * z2 + qw * x2;
        r[8] = qx * z2 + qw * y2;
        r[9] = qy * z2 - qw * x2;
        r[10] = 1.0F - (qx * x2 + qy * y2);
    }
    if (node->has_scale) {
        s[0] = node->scale[0];
        s[5] = node->scale[1];
        s[10] = node->scale[2];
    }
    Multiply(t, r, tr);
    Multiply(tr, s, out);
}

void GetMaterialColor(const cgltf_material* material, float* rgb) {
    rgb[0] = 0.75F;
    rgb[1] = 0.75F;
    rgb[2] = 0.78F;
    if (material && material->has_pbr_metallic_roughness) {
        const cgltf_float* base =
            material->pbr_metallic_roughness.base_color_factor;
        rgb[0] = base[0];
        rgb[1] = base[1];
        rgb[2] = base[2];
    }
}

void TireCompoundColor(int compound, float* rgb) {
    // Soft / Medium / Hard — readable tints on the dark preview.
    switch (compound) {
        case 0:
            rgb[0] = 0.82F;
            rgb[1] = 0.16F;
            rgb[2] = 0.20F;
            break;
        case 2:
            rgb[0] = 0.55F;
            rgb[1] = 0.56F;
            rgb[2] = 0.58F;
            break;
        default:
            rgb[0] = 0.08F;
            rgb[1] = 0.09F;
            rgb[2] = 0.10F;
            break;
    }
}

bool UnpackPositions(const cgltf_accessor* accessor, std::vector<float>* positions) {
    if (!accessor || accessor->type != cgltf_type_vec3 ||
        accessor->component_type != cgltf_component_type_r_32f) {
        return false;
    }
    positions->resize(accessor->count * 3);
    for (cgltf_size i = 0; i < accessor->count; ++i) {
        cgltf_accessor_read_float(accessor, i, positions->data() + i * 3, 3);
    }
    return true;
}

bool UnpackIndices(const cgltf_accessor* accessor, std::vector<uint32_t>* indices) {
    if (!accessor || accessor->type != cgltf_type_scalar) {
        return false;
    }
    indices->resize(accessor->count);
    for (cgltf_size i = 0; i < accessor->count; ++i) {
        (*indices)[i] =
            static_cast<uint32_t>(cgltf_accessor_read_index(accessor, i));
    }
    return true;
}

void BuildLocalMesh(
    const cgltf_mesh* mesh,
    const float* rest_world,
    bool grayscale_lighting,
    std::vector<VertexData>* vertices) {
    constexpr float kLightDir[3] = {0.3511F, 0.8026F, 0.4515F};
    constexpr float kAmbient = 0.38F;
    constexpr float kDiffuse = 0.72F;

    for (cgltf_size p = 0; p < mesh->primitives_count; ++p) {
        const cgltf_primitive& prim = mesh->primitives[p];
        if (prim.type != cgltf_primitive_type_triangles || !prim.indices) {
            continue;
        }
        const cgltf_accessor* position_accessor = nullptr;
        for (cgltf_size a = 0; a < prim.attributes_count; ++a) {
            if (prim.attributes[a].type == cgltf_attribute_type_position) {
                position_accessor = prim.attributes[a].data;
                break;
            }
        }
        if (!position_accessor) {
            continue;
        }

        std::vector<float> positions;
        std::vector<uint32_t> indices;
        if (!UnpackPositions(position_accessor, &positions) ||
            !UnpackIndices(prim.indices, &indices)) {
            continue;
        }

        float base_color[3];
        GetMaterialColor(prim.material, base_color);

        for (size_t t = 0; t + 2 < indices.size(); t += 3) {
            float local[3][3];
            bool valid = true;
            for (int c = 0; c < 3; ++c) {
                const uint32_t index = indices[t + static_cast<size_t>(c)];
                if (index * 3 + 2 >= positions.size()) {
                    valid = false;
                    break;
                }
                local[c][0] = positions[index * 3 + 0];
                local[c][1] = positions[index * 3 + 1];
                local[c][2] = positions[index * 3 + 2];
            }
            if (!valid) {
                continue;
            }

            const float e0[3] = {
                local[1][0] - local[0][0],
                local[1][1] - local[0][1],
                local[1][2] - local[0][2],
            };
            const float e1[3] = {
                local[2][0] - local[0][0],
                local[2][1] - local[0][1],
                local[2][2] - local[0][2],
            };
            float local_n[3] = {
                e0[1] * e1[2] - e0[2] * e1[1],
                e0[2] * e1[0] - e0[0] * e1[2],
                e0[0] * e1[1] - e0[1] * e1[0],
            };
            float world_n[3];
            TransformDirection(
                rest_world, local_n[0], local_n[1], local_n[2], world_n);
            Normalize3(world_n);
            const float shade =
                kAmbient + kDiffuse * std::abs(Dot3(world_n, kLightDir));

            for (int c = 0; c < 3; ++c) {
                if (grayscale_lighting) {
                    vertices->push_back(VertexData{
                        local[c][0],
                        local[c][1],
                        local[c][2],
                        shade,
                        shade,
                        shade,
                    });
                } else {
                    vertices->push_back(VertexData{
                        local[c][0],
                        local[c][1],
                        local[c][2],
                        std::min(1.0F, base_color[0] * shade),
                        std::min(1.0F, base_color[1] * shade),
                        std::min(1.0F, base_color[2] * shade),
                    });
                }
            }
        }
    }
}

SDL_GPUShader* LoadCubeShader(SDL_GPUDevice* device, bool is_vertex) {
    SDL_GPUShaderCreateInfo info{};
    info.num_uniform_buffers = is_vertex ? 1 : 0;
    info.stage =
        is_vertex ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
    const SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
        info.code = is_vertex ? cube_vert_dxil : cube_frag_dxil;
        info.code_size = is_vertex ? cube_vert_dxil_len : cube_frag_dxil_len;
        info.entrypoint = "main";
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = is_vertex ? cube_vert_msl : cube_frag_msl;
        info.code_size = is_vertex ? cube_vert_msl_len : cube_frag_msl_len;
        info.entrypoint = "main0";
    } else if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code = is_vertex ? cube_vert_spv : cube_frag_spv;
        info.code_size = is_vertex ? cube_vert_spv_len : cube_frag_spv_len;
        info.entrypoint = "main";
    } else {
        return nullptr;
    }
    return SDL_CreateGPUShader(device, &info);
}

}  // namespace

bool Viewport3D::IsWheelAssemblyName(const std::string& name) {
    return name.find("Tire") != std::string::npos ||
        name.find("Hub") != std::string::npos;
}

bool Viewport3D::Init(SDL_GPUDevice* device, const std::string& glb_path) {
    device_ = device;
    if (!device_) {
        return false;
    }

    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_ = SDL_CreateGPUSampler(device_, &sampler_info);
    if (!sampler_ || !CreatePipeline() || !LoadGlbOrCube(glb_path)) {
        Shutdown();
        return false;
    }

    RebuildWorldMatrices();
    ResolveFocusTarget(focus_center_, &target_distance_);
    anim_center_[0] = focus_center_[0];
    anim_center_[1] = focus_center_[1];
    anim_center_[2] = focus_center_[2];
    anim_distance_ = target_distance_;
    distance = full_car_distance_;
    ready_ = true;
    return true;
}

void Viewport3D::Shutdown() {
    DestroyTargets();
    DestroyDrawables();
    if (device_) {
        if (pipeline_) {
            SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
            pipeline_ = nullptr;
        }
        if (sampler_) {
            SDL_ReleaseGPUSampler(device_, sampler_);
            sampler_ = nullptr;
        }
    }
    nodes_.clear();
    root_nodes_.clear();
    world_node_ = -1;
    front_wing_pivot_ = -1;
    rear_wing_pivot_ = -1;
    binding_ = {};
    device_ = nullptr;
    ready_ = false;
}

void Viewport3D::DestroyDrawables() {
    if (device_) {
        for (Drawable& drawable : drawables_) {
            if (drawable.vertex_buffer) {
                SDL_ReleaseGPUBuffer(device_, drawable.vertex_buffer);
            }
        }
    }
    drawables_.clear();
}

void Viewport3D::DestroyTargets() {
    if (!device_) {
        return;
    }
    if (color_) {
        SDL_ReleaseGPUTexture(device_, color_);
        color_ = nullptr;
    }
    if (depth_) {
        SDL_ReleaseGPUTexture(device_, depth_);
        depth_ = nullptr;
    }
    binding_.texture = nullptr;
    binding_.sampler = sampler_;
    width_ = 0;
    height_ = 0;
}

int Viewport3D::FindNodeIndex(const char* name) const {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Viewport3D::CreateGpuVertexBuffer(
    const void* bytes,
    size_t byte_count,
    SDL_GPUBuffer** out_buffer) {
    SDL_GPUBufferCreateInfo buffer_info{};
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_info.size = static_cast<Uint32>(byte_count);
    *out_buffer = SDL_CreateGPUBuffer(device_, &buffer_info);
    if (!*out_buffer) {
        return false;
    }
    return WriteGpuVertexBuffer(*out_buffer, bytes, byte_count);
}

bool Viewport3D::WriteGpuVertexBuffer(
    SDL_GPUBuffer* buffer,
    const void* bytes,
    size_t byte_count) {
    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = static_cast<Uint32>(byte_count);
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device_, &transfer_info);
    if (!transfer) {
        return false;
    }
    void* mapped = SDL_MapGPUTransferBuffer(device_, transfer, false);
    std::memcpy(mapped, bytes, byte_count);
    SDL_UnmapGPUTransferBuffer(device_, transfer);

    SDL_GPUTransferBufferLocation source{};
    source.transfer_buffer = transfer;
    SDL_GPUBufferRegion destination{};
    destination.buffer = buffer;
    destination.size = static_cast<Uint32>(byte_count);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device_);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    SDL_UploadToGPUBuffer(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device_, transfer);
    return true;
}

void Viewport3D::ApplyTireCompound(int compound) {
    tire_compound_ = compound;
    float tint[3];
    TireCompoundColor(compound, tint);
    for (Drawable& drawable : drawables_) {
        if (!drawable.is_tire || drawable.tire_lit.empty() ||
            !drawable.vertex_buffer) {
            continue;
        }
        const size_t count = drawable.tire_lit.size() / 4;
        std::vector<VertexData> colored(count);
        for (size_t i = 0; i < count; ++i) {
            const float* src = drawable.tire_lit.data() + i * 4;
            const float shade = src[3];
            colored[i] = VertexData{
                src[0],
                src[1],
                src[2],
                std::min(1.0F, tint[0] * shade),
                std::min(1.0F, tint[1] * shade),
                std::min(1.0F, tint[2] * shade),
            };
        }
        WriteGpuVertexBuffer(
            drawable.vertex_buffer,
            colored.data(),
            colored.size() * sizeof(VertexData));
    }
}

bool Viewport3D::CreateCubeMesh() {
    nodes_.clear();
    root_nodes_.clear();
    DestroyDrawables();
    SceneNode node;
    node.name = "Cube";
    Identity(node.local);
    Identity(node.world);
    nodes_.push_back(node);
    root_nodes_.push_back(0);
    world_node_ = -1;
    front_wing_pivot_ = -1;
    rear_wing_pivot_ = -1;
    model_radius_ = 0.9F;
    min_distance_ = 1.2F;
    max_distance_ = 8.0F;
    full_car_distance_ = 2.6F;
    target_distance_ = full_car_distance_;

    Drawable drawable{};
    if (!CreateGpuVertexBuffer(
            kCubeVertices,
            sizeof(kCubeVertices),
            &drawable.vertex_buffer)) {
        return false;
    }
    drawable.vertex_count =
        static_cast<uint32_t>(sizeof(kCubeVertices) / sizeof(kCubeVertices[0]));
    drawable.node_index = 0;
    drawables_.push_back(std::move(drawable));
    return true;
}

bool Viewport3D::LoadGlbOrCube(const std::string& glb_path) {
    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, glb_path.c_str(), &data) !=
            cgltf_result_success ||
        cgltf_load_buffers(&options, data, glb_path.c_str()) !=
            cgltf_result_success) {
        if (data) {
            cgltf_free(data);
        }
        status = "Preview cube — vehicle GLB not found";
        return CreateCubeMesh();
    }

    nodes_.clear();
    root_nodes_.clear();
    DestroyDrawables();
    nodes_.resize(data->nodes_count);
    for (cgltf_size i = 0; i < data->nodes_count; ++i) {
        const cgltf_node* src = &data->nodes[i];
        SceneNode& dst = nodes_[i];
        dst.name = src->name ? src->name : ("node_" + std::to_string(i));
        GetNodeLocalMatrix(src, dst.local);
        Identity(dst.world);
        dst.parent = -1;
        dst.children.clear();
    }
    for (cgltf_size i = 0; i < data->nodes_count; ++i) {
        const cgltf_node* src = &data->nodes[i];
        for (cgltf_size c = 0; c < src->children_count; ++c) {
            const cgltf_size child_index =
                static_cast<cgltf_size>(src->children[c] - data->nodes);
            nodes_[i].children.push_back(static_cast<int>(child_index));
            nodes_[child_index].parent = static_cast<int>(i);
        }
    }

    const cgltf_scene* scene = data->scene ? data->scene : &data->scenes[0];
    if (!scene) {
        cgltf_free(data);
        status = "Preview cube — GLB has no scene";
        return CreateCubeMesh();
    }
    for (cgltf_size i = 0; i < scene->nodes_count; ++i) {
        root_nodes_.push_back(
            static_cast<int>(scene->nodes[i] - data->nodes));
    }

    front_wing_deg_ = rest_front_wing_deg_;
    rear_wing_deg_ = rest_rear_wing_deg_;
    front_ride_height_mm_ = rest_front_ride_mm_;
    rear_ride_height_mm_ = rest_rear_ride_mm_;
    RebuildWorldMatrices();

    float min_v[3] = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };
    float max_v[3] = {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };

    for (cgltf_size i = 0; i < data->nodes_count; ++i) {
        const cgltf_node* src = &data->nodes[i];
        if (!src->mesh) {
            continue;
        }
        const bool is_tire = nodes_[i].name.find("Tire") != std::string::npos;
        std::vector<VertexData> local_vertices;
        BuildLocalMesh(src->mesh, nodes_[i].world, is_tire, &local_vertices);
        if (local_vertices.empty()) {
            continue;
        }
        for (const VertexData& v : local_vertices) {
            float world[3];
            TransformPoint(nodes_[i].world, v.x, v.y, v.z, world);
            min_v[0] = std::min(min_v[0], world[0]);
            min_v[1] = std::min(min_v[1], world[1]);
            min_v[2] = std::min(min_v[2], world[2]);
            max_v[0] = std::max(max_v[0], world[0]);
            max_v[1] = std::max(max_v[1], world[1]);
            max_v[2] = std::max(max_v[2], world[2]);
        }

        Drawable drawable{};
        drawable.node_index = static_cast<int>(i);
        drawable.is_tire = is_tire;
        drawable.vertex_count = static_cast<uint32_t>(local_vertices.size());
        if (is_tire) {
            drawable.tire_lit.resize(local_vertices.size() * 4);
            for (size_t v = 0; v < local_vertices.size(); ++v) {
                drawable.tire_lit[v * 4 + 0] = local_vertices[v].x;
                drawable.tire_lit[v * 4 + 1] = local_vertices[v].y;
                drawable.tire_lit[v * 4 + 2] = local_vertices[v].z;
                drawable.tire_lit[v * 4 + 3] = local_vertices[v].red;
            }
        }
        if (!CreateGpuVertexBuffer(
                local_vertices.data(),
                local_vertices.size() * sizeof(VertexData),
                &drawable.vertex_buffer)) {
            cgltf_free(data);
            return false;
        }
        drawables_.push_back(std::move(drawable));
    }
    cgltf_free(data);

    if (drawables_.empty()) {
        status = "Preview cube — GLB had no meshes";
        return CreateCubeMesh();
    }

    const float center[3] = {
        0.5F * (min_v[0] + max_v[0]),
        0.5F * (min_v[1] + max_v[1]),
        0.5F * (min_v[2] + max_v[2]),
    };
    for (int root : root_nodes_) {
        nodes_[root].local[12] -= center[0];
        nodes_[root].local[13] -= center[1];
        nodes_[root].local[14] -= center[2];
        // If the scene root is "world", shift its children instead.
        if (nodes_[root].name == "world") {
            nodes_[root].local[12] += center[0];
            nodes_[root].local[13] += center[1];
            nodes_[root].local[14] += center[2];
            for (int child : nodes_[root].children) {
                nodes_[child].local[12] -= center[0];
                nodes_[child].local[13] -= center[1];
                nodes_[child].local[14] -= center[2];
            }
        }
    }

    const float ext[3] = {
        0.5F * (max_v[0] - min_v[0]),
        0.5F * (max_v[1] - min_v[1]),
        0.5F * (max_v[2] - min_v[2]),
    };
    model_radius_ =
        std::sqrt(ext[0] * ext[0] + ext[1] * ext[1] + ext[2] * ext[2]);
    full_car_distance_ = std::max(4.5F, model_radius_ * 3.4F);
    target_distance_ = full_car_distance_;
    min_distance_ = std::max(1.6F, model_radius_ * 1.1F);
    max_distance_ = std::max(full_car_distance_ * 2.8F, model_radius_ * 7.0F);
    distance = full_car_distance_;

    world_node_ = FindNodeIndex("world");
    front_wing_pivot_ = FindNodeIndex("FrontWingPivot");
    rear_wing_pivot_ = FindNodeIndex("RearWingPivot");
    ApplyTireCompound(tire_compound_);
    RebuildWorldMatrices();
    status = "Hover setup controls to focus · Reset restores the view";
    return true;
}

void Viewport3D::RebuildWorldMatrices() {
    constexpr float kWingGain = 1.8F;
    // mm delta → meters, amplified so ride height is obvious in preview.
    constexpr float kRideMmToVisual = 0.0045F;
    const float front_delta =
        (front_wing_deg_ - rest_front_wing_deg_) * kWingGain;
    const float rear_delta = (rear_wing_deg_ - rest_rear_wing_deg_) * kWingGain;
    const float front_ride =
        (front_ride_height_mm_ - rest_front_ride_mm_) * kRideMmToVisual;
    const float rear_ride =
        (rear_ride_height_mm_ - rest_rear_ride_mm_) * kRideMmToVisual;

    struct Item {
        int index;
        bool has_parent_world;
        float parent_world[16];
    };
    std::vector<Item> items;
    for (int root : root_nodes_) {
        Item item{};
        item.index = root;
        item.has_parent_world = false;
        Identity(item.parent_world);
        items.push_back(item);
    }

    size_t cursor = 0;
    while (cursor < items.size()) {
        const Item current = items[cursor++];
        SceneNode& node = nodes_[current.index];
        float authored[16];
        float effective[16];
        CopyMatrix(node.local, authored);

        // Ride height: lift body parts under world; wheels stay planted.
        // Blend front/rear by local Z (positive Z is toward the nose).
        if (node.parent == world_node_ && !IsWheelAssemblyName(node.name)) {
            const float z = authored[14];
            const float t = std::clamp((z + 1.3F) / 2.6F, 0.0F, 1.0F);
            authored[13] += Lerp(rear_ride, front_ride, t);
        }

        CopyMatrix(authored, effective);
        if (current.has_parent_world) {
            const int parent = node.parent;
            const bool is_pylon = node.name.find("Pylon") != std::string::npos;
            float hinge[16];
            if (parent == front_wing_pivot_ && !is_pylon &&
                std::fabs(front_delta) > 1.0e-4F) {
                RotationX(front_delta, hinge);
                Multiply(hinge, authored, effective);
            } else if (
                parent == rear_wing_pivot_ && !is_pylon &&
                std::fabs(rear_delta) > 1.0e-4F) {
                RotationX(rear_delta, hinge);
                Multiply(hinge, authored, effective);
            }
            Multiply(current.parent_world, effective, node.world);
        } else {
            CopyMatrix(effective, node.world);
        }

        for (int child : node.children) {
            Item next{};
            next.index = child;
            next.has_parent_world = true;
            CopyMatrix(node.world, next.parent_world);
            items.push_back(next);
        }
    }
}

void Viewport3D::ResolveFocusTarget(float out_center[3], float* out_distance) const {
    out_center[0] = 0.0F;
    out_center[1] = 0.0F;
    out_center[2] = 0.0F;
    *out_distance = full_car_distance_;

    auto focus_node = [&](const char* name, float scale) {
        const int index = FindNodeIndex(name);
        if (index < 0) {
            return false;
        }
        out_center[0] = nodes_[index].world[12];
        out_center[1] = nodes_[index].world[13];
        out_center[2] = nodes_[index].world[14];
        *out_distance = std::max(min_distance_, full_car_distance_ * scale);
        return true;
    };

    switch (focus_) {
        case ViewportFocus::FrontWing:
            focus_node("FrontWingPivot", 0.45F);
            break;
        case ViewportFocus::RearWing:
            focus_node("RearWingPivot", 0.45F);
            break;
        case ViewportFocus::FrontRideHeight:
            if (!focus_node("Nose", 0.55F)) {
                focus_node("FrontSplitter", 0.55F);
            }
            break;
        case ViewportFocus::RearRideHeight:
            if (!focus_node("RearBumper", 0.55F)) {
                focus_node("RearWingMainPlane", 0.55F);
            }
            break;
        case ViewportFocus::Tires:
            if (!focus_node("FrontLeftTire", 0.50F)) {
                focus_node("FrontRightTire", 0.50F);
            }
            break;
        case ViewportFocus::FullCar:
        default:
            break;
    }
}

void Viewport3D::UpdateFromVehicle(const Vehicle& vehicle) {
    front_wing_deg_ = vehicle.front_wing_deg;
    rear_wing_deg_ = vehicle.rear_wing_deg;
    front_ride_height_mm_ = vehicle.front_ride_height_mm;
    rear_ride_height_mm_ = vehicle.rear_ride_height_mm;
    if (vehicle.compound != tire_compound_) {
        ApplyTireCompound(vehicle.compound);
    }
    RebuildWorldMatrices();
    ResolveFocusTarget(focus_center_, &target_distance_);
}

void Viewport3D::SetFocus(ViewportFocus focus) {
    focus_ = focus;
    ResolveFocusTarget(focus_center_, &target_distance_);
}

void Viewport3D::ResetView() {
    focus_ = ViewportFocus::FullCar;
    yaw_deg = default_yaw_deg_;
    pitch_deg = default_pitch_deg_;
    distance = full_car_distance_;
    target_distance_ = full_car_distance_;
    focus_center_[0] = 0.0F;
    focus_center_[1] = 0.0F;
    focus_center_[2] = 0.0F;
    anim_center_[0] = 0.0F;
    anim_center_[1] = 0.0F;
    anim_center_[2] = 0.0F;
    anim_distance_ = full_car_distance_;
}

bool Viewport3D::CreatePipeline() {
    SDL_GPUShader* vertex = LoadCubeShader(device_, true);
    SDL_GPUShader* fragment = LoadCubeShader(device_, false);
    if (!vertex || !fragment) {
        if (vertex) {
            SDL_ReleaseGPUShader(device_, vertex);
        }
        if (fragment) {
            SDL_ReleaseGPUShader(device_, fragment);
        }
        return false;
    }

    SDL_GPUVertexBufferDescription vertex_buffer_desc{};
    vertex_buffer_desc.slot = 0;
    vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_desc.pitch = sizeof(VertexData);

    SDL_GPUVertexAttribute vertex_attributes[2]{};
    vertex_attributes[0] = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0};
    vertex_attributes[1] = {
        1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(float) * 3};

    SDL_GPUColorTargetDescription color_target{};
    color_target.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.vertex_shader = vertex;
    pipeline_info.fragment_shader = fragment;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions =
        &vertex_buffer_desc;
    pipeline_info.vertex_input_state.num_vertex_attributes = 2;
    pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipeline_info.rasterizer_state.front_face =
        SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeline_info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    pipeline_info.depth_stencil_state.enable_depth_test = true;
    pipeline_info.depth_stencil_state.enable_depth_write = true;
    pipeline_info.depth_stencil_state.compare_op =
        SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_target;
    pipeline_info.target_info.has_depth_stencil_target = true;
    pipeline_info.target_info.depth_stencil_format =
        SDL_GPU_TEXTUREFORMAT_D16_UNORM;

    pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pipeline_info);
    SDL_ReleaseGPUShader(device_, vertex);
    SDL_ReleaseGPUShader(device_, fragment);
    return pipeline_ != nullptr;
}

void Viewport3D::EnsureSize(int width, int height) {
    if (!ready_ || width <= 0 || height <= 0) {
        return;
    }
    if (width == width_ && height == height_ && color_ && depth_) {
        return;
    }
    DestroyTargets();
    width_ = width;
    height_ = height;

    SDL_GPUTextureCreateInfo color_info{};
    color_info.type = SDL_GPU_TEXTURETYPE_2D;
    color_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    color_info.usage =
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    color_info.width = static_cast<Uint32>(width_);
    color_info.height = static_cast<Uint32>(height_);
    color_info.layer_count_or_depth = 1;
    color_info.num_levels = 1;
    color_ = SDL_CreateGPUTexture(device_, &color_info);

    SDL_GPUTextureCreateInfo depth_info{};
    depth_info.type = SDL_GPU_TEXTURETYPE_2D;
    depth_info.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    depth_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depth_info.width = static_cast<Uint32>(width_);
    depth_info.height = static_cast<Uint32>(height_);
    depth_info.layer_count_or_depth = 1;
    depth_info.num_levels = 1;
    depth_ = SDL_CreateGPUTexture(device_, &depth_info);
    if (!color_ || !depth_) {
        DestroyTargets();
        return;
    }
    binding_.texture = color_;
    binding_.sampler = sampler_;
}

void Viewport3D::Render(SDL_GPUCommandBuffer* command_buffer) {
    if (!ready_ || !command_buffer || !color_ || !depth_ || !pipeline_ ||
        drawables_.empty() || width_ <= 0 || height_ <= 0) {
        return;
    }

    constexpr float kCamLerp = 0.12F;
    anim_center_[0] = Lerp(anim_center_[0], focus_center_[0], kCamLerp);
    anim_center_[1] = Lerp(anim_center_[1], focus_center_[1], kCamLerp);
    anim_center_[2] = Lerp(anim_center_[2], focus_center_[2], kCamLerp);
    anim_distance_ = Lerp(anim_distance_, target_distance_, kCamLerp);
    const float orbit_distance =
        std::clamp(distance, min_distance_, max_distance_);
    const float cam_distance =
        (focus_ == ViewportFocus::FullCar) ? orbit_distance : anim_distance_;

    float view[16];
    float proj[16];
    float vp[16];
    const float yaw = yaw_deg * kPi / 180.0F;
    const float pitch = pitch_deg * kPi / 180.0F;
    const float cos_pitch = std::cos(pitch);
    const float eye[3] = {
        anim_center_[0] + cam_distance * std::sin(yaw) * cos_pitch,
        anim_center_[1] + cam_distance * std::sin(pitch),
        anim_center_[2] + cam_distance * std::cos(yaw) * cos_pitch,
    };
    const float up[3] = {0.0F, 1.0F, 0.0F};
    LookAt(eye, anim_center_, up, view);
    Perspective(
        45.0F,
        static_cast<float>(width_) / static_cast<float>(height_),
        0.05F,
        100.0F,
        proj);
    Multiply(proj, view, vp);

    SDL_GPUColorTargetInfo color_target{};
    color_target.texture = color_;
    color_target.clear_color = SDL_FColor{0.07F, 0.08F, 0.10F, 1.0F};
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;
    color_target.cycle = true;

    SDL_GPUDepthStencilTargetInfo depth_target{};
    depth_target.texture = depth_;
    depth_target.clear_depth = 1.0F;
    depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth_target.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth_target.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth_target.cycle = true;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(command_buffer, &color_target, 1, &depth_target);
    SDL_BindGPUGraphicsPipeline(pass, pipeline_);
    for (const Drawable& drawable : drawables_) {
        if (!drawable.vertex_buffer || drawable.vertex_count == 0 ||
            drawable.node_index < 0 ||
            drawable.node_index >= static_cast<int>(nodes_.size())) {
            continue;
        }
        float mvp[16];
        Multiply(vp, nodes_[drawable.node_index].world, mvp);
        SDL_PushGPUVertexUniformData(command_buffer, 0, mvp, sizeof(mvp));
        SDL_GPUBufferBinding vertex_binding{};
        vertex_binding.buffer = drawable.vertex_buffer;
        SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);
        SDL_DrawGPUPrimitives(pass, drawable.vertex_count, 1, 0, 0);
    }
    SDL_EndGPURenderPass(pass);
}

ImTextureID Viewport3D::TextureId() const {
    return reinterpret_cast<ImTextureID>(
        const_cast<SDL_GPUTextureSamplerBinding*>(&binding_));
}

bool Viewport3D::HasTexture() const {
    return binding_.texture != nullptr && binding_.sampler != nullptr;
}

void Viewport3D::Orbit(float dx_pixels, float dy_pixels) {
    yaw_deg -= dx_pixels * 0.35F;
    pitch_deg += dy_pixels * 0.35F;
    pitch_deg = std::clamp(pitch_deg, -89.0F, 89.0F);
}

void Viewport3D::Zoom(float wheel) {
    distance *= (1.0F - wheel * 0.12F);
    distance = std::clamp(distance, min_distance_, max_distance_);
    if (focus_ == ViewportFocus::FullCar) {
        full_car_distance_ = distance;
        target_distance_ = distance;
        anim_distance_ = distance;
    }
}
