#include "pkgi.hpp"
extern "C"
{
#include "style.h"
}

#include "imgui.hpp"

#include <vita2d.h>

extern SceGxmProgram _binary_assets_imgui_v_cg_gxp_start;
extern SceGxmProgram _binary_assets_imgui_f_cg_gxp_start;

namespace
{
void matrix_init_orthographic(
        float* m,
        float left,
        float right,
        float bottom,
        float top,
        float near,
        float far)
{
    m[0x0] = 2.0f / (right - left);
    m[0x4] = 0.0f;
    m[0x8] = 0.0f;
    m[0xC] = -(right + left) / (right - left);

    m[0x1] = 0.0f;
    m[0x5] = 2.0f / (top - bottom);
    m[0x9] = 0.0f;
    m[0xD] = -(top + bottom) / (top - bottom);

    m[0x2] = 0.0f;
    m[0x6] = 0.0f;
    m[0xA] = -2.0f / (far - near);
    m[0xE] = (far + near) / (far - near);

    m[0x3] = 0.0f;
    m[0x7] = 0.0f;
    m[0xB] = 0.0f;
    m[0xF] = 1.0f;
}

SceGxmShaderPatcherId imguiVertexProgramId;
SceGxmShaderPatcherId imguiFragmentProgramId;

SceGxmVertexProgram* _vita2d_imguiVertexProgram;
SceGxmFragmentProgram* _vita2d_imguiFragmentProgram;

const SceGxmProgramParameter* _vita2d_imguiWvpParam;

float ortho_proj_matrix[16];

constexpr auto ImguiVertexSize = 20;
}

void init_imgui()
{
    uint32_t err;

    // check the shaders
    err = sceGxmProgramCheck(&_binary_assets_imgui_v_cg_gxp_start);
    if (err != 0)
        throw formatEx<std::runtime_error>(
                "imgui_v sceGxmProgramCheck(): {:#08x}",
                static_cast<uint32_t>(err));
    err = sceGxmProgramCheck(&_binary_assets_imgui_f_cg_gxp_start);
    if (err != 0)
        throw formatEx<std::runtime_error>(
                "imgui_f sceGxmProgramCheck(): {:#08x}",
                static_cast<uint32_t>(err));

    // register programs with the patcher
    err = sceGxmShaderPatcherRegisterProgram(
            vita2d_get_shader_patcher(),
            &_binary_assets_imgui_v_cg_gxp_start,
            &imguiVertexProgramId);
    LOG("imgui_v sceGxmShaderPatcherRegisterProgram(): 0x%08X\n", err);

    err = sceGxmShaderPatcherRegisterProgram(
            vita2d_get_shader_patcher(),
            &_binary_assets_imgui_f_cg_gxp_start,
            &imguiFragmentProgramId);
    LOG("imgui_f sceGxmShaderPatcherRegisterProgram(): 0x%08X\n", err);

    // get attributes by name to create vertex format bindings
    const SceGxmProgramParameter* paramTexturePositionAttribute =
            sceGxmProgramFindParameterByName(
                    &_binary_assets_imgui_v_cg_gxp_start, "aPosition");
    LOG("aPosition sceGxmProgramFindParameterByName(): %p\n",
        paramTexturePositionAttribute);

    const SceGxmProgramParameter* paramTextureTexcoordAttribute =
            sceGxmProgramFindParameterByName(
                    &_binary_assets_imgui_v_cg_gxp_start, "aTexcoord");
    LOG("aTexcoord sceGxmProgramFindParameterByName(): %p\n",
        paramTextureTexcoordAttribute);

    const SceGxmProgramParameter* paramTextureColorAttribute =
            sceGxmProgramFindParameterByName(
                    &_binary_assets_imgui_v_cg_gxp_start, "aColor");
    LOG("aColor sceGxmProgramFindParameterByName(): %p\n",
        paramTextureColorAttribute);

    // create texture vertex format
    SceGxmVertexAttribute textureVertexAttributes[3];
    SceGxmVertexStream textureVertexStreams[1];
    /* x,y,z: 3 float 32 bits */
    textureVertexAttributes[0].streamIndex = 0;
    textureVertexAttributes[0].offset = 0;
    textureVertexAttributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    textureVertexAttributes[0].componentCount = 2; // (x, y)
    textureVertexAttributes[0].regIndex =
            sceGxmProgramParameterGetResourceIndex(
                    paramTexturePositionAttribute);
    /* u,v: 2 floats 32 bits */
    textureVertexAttributes[1].streamIndex = 0;
    textureVertexAttributes[1].offset = 8; // (x, y) * 4 = 12 bytes
    textureVertexAttributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    textureVertexAttributes[1].componentCount = 2; // (u, v)
    textureVertexAttributes[1].regIndex =
            sceGxmProgramParameterGetResourceIndex(
                    paramTextureTexcoordAttribute);
    /* r,g,b,a: 4 int 8 bits */
    textureVertexAttributes[2].streamIndex = 0;
    textureVertexAttributes[2].offset =
            16; // (x, y) * 4 + (u, v) * 4 = 20 bytes
    textureVertexAttributes[2].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
    textureVertexAttributes[2].componentCount = 4; // (r, g, b, a)
    textureVertexAttributes[2].regIndex =
            sceGxmProgramParameterGetResourceIndex(paramTextureColorAttribute);
    // 16 bit (short) indices
    textureVertexStreams[0].stride = ImguiVertexSize;
    textureVertexStreams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

    // create texture shaders
    err = sceGxmShaderPatcherCreateVertexProgram(
            vita2d_get_shader_patcher(),
            imguiVertexProgramId,
            textureVertexAttributes,
            3,
            textureVertexStreams,
            1,
            &_vita2d_imguiVertexProgram);

    LOG("imgui sceGxmShaderPatcherCreateVertexProgram(): 0x%08X\n", err);

    // Fill SceGxmBlendInfo
    static SceGxmBlendInfo blend_info{};
    blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
    blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
    blend_info.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
    blend_info.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_info.alphaSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
    blend_info.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;

    err = sceGxmShaderPatcherCreateFragmentProgram(
            vita2d_get_shader_patcher(),
            imguiFragmentProgramId,
            SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
            SCE_GXM_MULTISAMPLE_NONE,
            &blend_info,
            &_binary_assets_imgui_v_cg_gxp_start,
            &_vita2d_imguiFragmentProgram);

    LOG("imgui sceGxmShaderPatcherCreateFragmentProgram(): 0x%08X\n", err);

    // find vertex uniforms by name and cache parameter information
    _vita2d_imguiWvpParam = sceGxmProgramFindParameterByName(
            &_binary_assets_imgui_v_cg_gxp_start, "wvp");
    LOG("imgui wvp sceGxmProgramFindParameterByName(): %p\n",
        _vita2d_imguiWvpParam);

    matrix_init_orthographic(
            ortho_proj_matrix, 0.0f, VITA_WIDTH, VITA_HEIGHT, 0.0f, 0.0f, 1.0f);
}

void pkgi_imgui_render(ImDrawData* draw_data)
{
    auto _vita2d_context = vita2d_get_context();

    sceGxmSetVertexProgram(_vita2d_context, _vita2d_imguiVertexProgram);
    sceGxmSetFragmentProgram(_vita2d_context, _vita2d_imguiFragmentProgram);

    void* vertex_wvp_buffer;
    sceGxmReserveVertexDefaultUniformBuffer(
            vita2d_get_context(), &vertex_wvp_buffer);
    sceGxmSetUniformDataF(
            vertex_wvp_buffer, _vita2d_imguiWvpParam, 0, 16, ortho_proj_matrix);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const auto cmd_list = draw_data->CmdLists[n];
        const auto vtx_buffer = cmd_list->VtxBuffer.Data;
        const auto vtx_size = cmd_list->VtxBuffer.Size;
        const auto idx_buffer = cmd_list->IdxBuffer.Data;
        const auto idx_size = cmd_list->IdxBuffer.Size;

        const auto vertices = vita2d_pool_memalign(
                vtx_size * ImguiVertexSize, ImguiVertexSize);
        if (vertices == nullptr)
            throw std::runtime_error("video memory too low");
        memcpy(vertices, vtx_buffer, vtx_size * ImguiVertexSize);

        static_assert(sizeof(ImDrawIdx) == 2);
        auto indices = (uint16_t*)vita2d_pool_memalign(
                idx_size * sizeof(ImDrawIdx), sizeof(void*));
        if (vertices == nullptr)
            throw std::runtime_error("video memory too low");
        memcpy(indices, idx_buffer, idx_size * sizeof(ImDrawIdx));

        auto err = sceGxmSetVertexStream(_vita2d_context, 0, vertices);
        if (err != 0)
            throw formatEx<std::runtime_error>(
                    "sceGxmSetVertexStream failed: {:#08x}",
                    static_cast<uint32_t>(err));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const auto pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                const auto texture = (vita2d_texture*)pcmd->TextureId;
                err = sceGxmSetFragmentTexture(
                        _vita2d_context, 0, &texture->gxm_tex);
                if (err != 0)
                    throw formatEx<std::runtime_error>(
                            "sceGxmSetFragmentTexture failed: {:#08x}",
                            static_cast<uint32_t>(err));

                err = sceGxmDraw(
                        _vita2d_context,
                        SCE_GXM_PRIMITIVE_TRIANGLES,
                        SCE_GXM_INDEX_FORMAT_U16,
                        indices,
                        pcmd->ElemCount);
                if (err != 0)
                    throw formatEx<std::runtime_error>(
                            "sceGxmDraw failed: {:#08x}",
                            static_cast<uint32_t>(err));
            }
            indices += pcmd->ElemCount;
        }
    }
}