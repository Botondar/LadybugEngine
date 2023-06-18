#include "LadybugEngine.hpp"

#include "LadybugLib/LBLibBuild.cpp"
//#include "Renderer/Renderer.cpp"

#include "Debug.cpp"
#include "Font.cpp"
#include "World.cpp"
#include "Editor.cpp"

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable:4244)
#include "stb/stb_image.h"
#pragma warning(pop)

internal void LoadDebugFont(game_state* GameState, const char* Path);
lbfn void RenderMeshes(VkCommandBuffer CmdBuffer, VkPipelineLayout PipelineLayout, 
                       geometry_buffer* GeometryBuffer, game_state* GameState,
                       const frustum* Frustum = nullptr, bool DoCulling = false);

lbfn bool IntersectFrustum(const frustum* Frustum, const mmbox* Box)
{
    bool Result = true;

    v3 HalfExtent = 0.5f * (Box->Max - Box->Min);
    if (Dot(HalfExtent, HalfExtent) > 1e-6f)
    {
        v3 CenterP3 = 0.5f * (Box->Min + Box->Max);
        v4 CenterP = { CenterP3.x, CenterP3.y, CenterP3.z, 1.0f };

        for (u32 i = 0; i < 6; i++)
        {
            f32 EffectiveRadius = 
                Abs(Frustum->Planes[i].x * HalfExtent.x) + 
                Abs(Frustum->Planes[i].y * HalfExtent.y) +
                Abs(Frustum->Planes[i].z * HalfExtent.z);

            if (Dot(CenterP, Frustum->Planes[i]) < -EffectiveRadius)
            {
                Result = false;
                break;
            }
        }
    }
    return Result;
}

internal void LoadDebugFont(game_state* GameState, const char* Path)
{
    buffer FileBuffer = Platform.LoadEntireFile(Path, &GameState->TransientArena);
    lbfnt_file* FontFile = (lbfnt_file*)FileBuffer.Data;

    if (FontFile)
    {
        // HACK(boti): When sampling with wrap+linear mode, UV 0,0 pulls in all 4 corner texels,
        // so we set those to the maximum alpha value here.
        {
            auto SetPixel = [&](u32 x, u32 y, u8 Value) { FontFile->Bitmap.Bitmap[x + y*FontFile->Bitmap.Width] = Value; };
            u32 x = FontFile->Bitmap.Width - 1;
            u32 y = FontFile->Bitmap.Height - 1;
            SetPixel(0, 0, 0xFFu);
            SetPixel(x, 0, 0xFFu);
            SetPixel(0, y, 0xFFu);
            SetPixel(x, y, 0xFFu);
        }
        
        if (FontFile->FileTag == LBFNT_FILE_TAG)
        {
            font* Font = &GameState->Assets->DefaultFont;
            Font->RasterHeight = FontFile->FontInfo.RasterHeight;
            Font->Ascent = FontFile->FontInfo.Ascent;
            Font->Descent = FontFile->FontInfo.Descent;
            Font->BaselineDistance = FontFile->FontInfo.BaselineDistance;

            for (u32 i = 0; i < Font->CharCount; i++)
            {
                Font->CharMapping[i] = 
                {
                    .Codepoint = FontFile->CharacterMap[i].Codepoint,
                    .GlyphIndex = FontFile->CharacterMap[i].GlyphIndex,
                };
            }

            Font->GlyphCount = FontFile->GlyphCount;
            Assert(Font->GlyphCount <= Font->MaxGlyphCount);

            for (u32 i = 0; i < Font->GlyphCount; i++)
            {
                lbfnt_glyph* Glyph = FontFile->Glyphs + i;
                Font->Glyphs[i] = 
                {
                    .UV0 = { Glyph->u0, Glyph->v0 },
                    .UV1 = { Glyph->u1, Glyph->v1 },
                    .P0 = { Glyph->BoundsLeft, Glyph->BoundsTop },
                    .P1 = { Glyph->BoundsRight, Glyph->BoundsBottom },
                    .AdvanceX = Glyph->AdvanceX,
                    .OriginY = Glyph->OriginY,
                };

                // TODO(boti): This is the way Direct2D specifies that the bounds are empty,
                //             we probably shouldn't be exporting the glyph bounds this way in the first place!
                if (Font->Glyphs[i].P0.x > Font->Glyphs[i].P1.x)
                {
                    Font->Glyphs[i].P0 = { 0.0f, 0.0f };
                    Font->Glyphs[i].P1 = { 0.0f, 0.0f };
                }
            }

            GameState->Assets->DefaultFontTextureID =  PushTexture(GameState->Renderer, 
                FontFile->Bitmap.Width, FontFile->Bitmap.Height, 1, 
                FontFile->Bitmap.Bitmap, VK_FORMAT_R8_UNORM, 
                {
                    .R = Swizzle_One,
                    .G = Swizzle_One,
                    .B = Swizzle_One,
                    .A = Swizzle_R,
                });
        }
        else
        {
            UnhandledError("Invalid font file");
        }
    }
    else
    {
        UnimplementedCodePath;
    }
}

lbfn void RenderMeshes(VkCommandBuffer CmdBuffer, VkPipelineLayout PipelineLayout, 
                       geometry_buffer* GeometryBuffer, game_state* GameState,
                       const frustum* Frustum /*= nullptr*/, bool DoCulling /*= false*/)
{
    assets* Assets = GameState->Assets;
    game_world* World = GameState->World;;

    VkDeviceSize Offset = 0;
    vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &GeometryBuffer->VertexMemory.Buffer, &Offset);
    vkCmdBindIndexBuffer(CmdBuffer, GeometryBuffer->IndexMemory.Buffer, 0, VK_INDEX_TYPE_UINT32);
    for (u32 i = 0; i < World->InstanceCount; i++)
    {
        const mesh_instance* Instance = World->Instances + i;
        geometry_buffer_allocation Allocation = Assets->Meshes[Instance->MeshID];
        mmbox Box = Assets->MeshBoxes[Instance->MeshID];
        u32 MaterialIndex = Assets->MeshMaterialIndices[Instance->MeshID];

        v4 BoxMin = Instance->Transform * v4{ Box.Min.x, Box.Min.y, Box.Min.z, 1.0f };
        v4 BoxMax = Instance->Transform * v4{ Box.Max.x, Box.Max.y, Box.Max.z, 1.0f };

        Box.Min = { BoxMin.x, BoxMin.y, BoxMin.z };
        Box.Max = { BoxMax.x, BoxMax.y, BoxMax.z };

        bool IsVisible = true;
        if (DoCulling)
        {
            IsVisible = IntersectFrustum(Frustum, &Box);
        }

        if (IsVisible)
        {
            push_constants PushConstants = {};
            PushConstants.Transform = Instance->Transform;
            PushConstants.Material = Assets->Materials[MaterialIndex];

            vkCmdPushConstants(CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, 
                               0, sizeof(PushConstants), &PushConstants);

            if (Allocation.IndexBlock)
            {
                u32 IndexCount = (u32)(Allocation.IndexBlock->ByteSize / sizeof(vert_index));
                u32 IndexOffset = (u32)(Allocation.IndexBlock->ByteOffset / sizeof(vert_index));
                u32 VertexOffset = (u32)(Allocation.VertexBlock->ByteOffset / sizeof(vertex));
                vkCmdDrawIndexed(CmdBuffer, IndexCount, 1, IndexOffset, VertexOffset, 0);
            }
            else
            {
                u32 VertexCount = (u32)(Allocation.VertexBlock->ByteSize / sizeof(vertex));
                u32 VertexOffset = (u32)(Allocation.VertexBlock->ByteOffset / sizeof(vertex));
                vkCmdDraw(CmdBuffer, VertexCount, 1, VertexOffset, 0);
            }
        }
    }
}

internal void GameRender(game_state* GameState, render_frame* Frame)
{
    renderer* Renderer = GameState->Renderer;
    
    game_world* World = GameState->World;
    assets* Assets = GameState->Assets;

    m4 CameraTransform = GetTransform(&World->Camera);
    m4 ViewTransform = AffineOrthonormalInverse(CameraTransform);
    render_camera Camera = 
    {
        .CameraTransform = CameraTransform,
        .ViewTransform = ViewTransform,
        .FocalLength = 1.0f / Tan(0.5f * World->Camera.FieldOfView),
        .NearZ = World->Camera.NearZ,
        .FarZ = World->Camera.FarZ,
    };

    SetRenderCamera(Frame, &Camera);
    SetLights(Frame, World->SunV, World->SunL);

    BeginSceneRendering(Frame);

    VkDescriptorSet OcclusionDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_SampledRenderTargetPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->OcclusionBuffers[1]->View,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); 

    VkDescriptorSet StructureBufferDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_SampledRenderTargetPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->StructureBuffer->View,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); 

    VkDescriptorSet ShadowDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_ShadowPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->ShadowMapView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkViewport FrameViewport = 
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (f32)Renderer->SurfaceExtent.width,
        .height = (f32)Renderer->SurfaceExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    VkRect2D FrameScissor = 
    {
        .offset = { 0, 0 },
        .extent = Renderer->SurfaceExtent,
    };

    // 3D render
    {
        const VkDescriptorSet DescriptorSets[] = 
        {
            Renderer->TextureManager.DescriptorSets[0], // Samplers
            Renderer->TextureManager.DescriptorSets[1], // Images
            Frame->UniformDescriptorSet,
            OcclusionDescriptorSet,
            StructureBufferDescriptorSet,
            ShadowDescriptorSet,
        };

        VkViewport ShadowViewport = 
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = (f32)R_ShadowResolution,
            .height = (f32)R_ShadowResolution,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        VkRect2D ShadowScissor = 
        {
            .offset = { 0, 0 },
            .extent = { R_ShadowResolution, R_ShadowResolution },
        };
        vkCmdSetViewport(Frame->CmdBuffer, 0, 1, &ShadowViewport);
        vkCmdSetScissor(Frame->CmdBuffer, 0, 1, &ShadowScissor);
        for (u32 Cascade = 0; Cascade < R_MaxShadowCascadeCount; Cascade++)
        {
            BeginCascade(Frame, Cascade);

            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Shadow];
            vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdPushConstants(Frame->CmdBuffer, Pipeline.Layout,
                               VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                               sizeof(push_constants), sizeof(u32), &Cascade);
            vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout,
                                    0, 3, DescriptorSets, 0, nullptr);
            RenderMeshes(Frame->CmdBuffer, Pipeline.Layout,
                         &Renderer->GeometryBuffer, GameState);

            EndCascade(Frame);
        }

        bool EnablePrimaryCull = false;
        vkCmdSetViewport(Frame->CmdBuffer, 0, 1, &FrameViewport);
        vkCmdSetScissor(Frame->CmdBuffer, 0, 1, &FrameScissor);
        BeginPrepass(Frame);
        {
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Prepass];
            vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout, 
                                    0, 3, DescriptorSets,
                                    0, nullptr);
            RenderMeshes(Frame->CmdBuffer, Pipeline.Layout,
                         &Renderer->GeometryBuffer, GameState,
                         &Frame->CameraFrustum, EnablePrimaryCull);
        }
        EndPrepass(Frame);

        RenderSSAO(Frame,
                   GameState->PostProcessParams.SSAO,
                   Renderer->Pipelines[Pipeline_SSAO].Pipeline, 
                   Renderer->Pipelines[Pipeline_SSAO].Layout, 
                   Renderer->SetLayouts[SetLayout_SSAO],
                   Renderer->Pipelines[Pipeline_SSAOBlur].Pipeline, 
                   Renderer->Pipelines[Pipeline_SSAOBlur].Layout, 
                   Renderer->SetLayouts[SetLayout_SSAOBlur]);

        BeginForwardPass(Frame);
        {
            // Bloom testing
#if 0
            {
                f32 PixelSizeX = 1.0f / Frame->RenderExtent.width;
                f32 PixelSizeY = 1.0f / Frame->RenderExtent.height;
                pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Quad];
                struct data
                {
                    v2 P;
                    v2 HalfExtent;
                    v3 Color;
                };
                data Data = 
                {
                    .P = { 0.0f, 0.0f }, 
                    .HalfExtent = { 30.0f * PixelSizeX, 30.0f * PixelSizeY },
                    .Color = 1000.0f * v3{ 1.0f, 0.0f, 0.0f },
                };
                vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
                vkCmdPushConstants(Frame->CmdBuffer, Pipeline.Layout, VK_SHADER_STAGE_ALL, 0, sizeof(data), &Data);
                vkCmdDraw(Frame->CmdBuffer, 6, 1, 0, 0);
            }
#endif

            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Simple];
            vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout, 
                                    0, CountOf(DescriptorSets), DescriptorSets,
                                    0, nullptr);

            RenderMeshes(Frame->CmdBuffer, Pipeline.Layout,
                         &Renderer->GeometryBuffer, GameState,
                         &Frame->CameraFrustum, EnablePrimaryCull);

            // Render sky
#if 1
            {
                pipeline_with_layout SkyPipeline = Renderer->Pipelines[Pipeline_Sky];
                vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Pipeline);
                vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Layout, 
                                        0, 1, &Frame->UniformDescriptorSet, 0, nullptr);
                vkCmdDraw(Frame->CmdBuffer, 3, 1, 0, 0);
            }
#endif
        }
        EndForwardPass(Frame);
    }
    EndSceneRendering(Frame);

    RenderBloom(Frame,
                GameState->PostProcessParams.Bloom,
                Frame->HDRRenderTargets[0],
                Frame->HDRRenderTargets[1],
                Renderer->Pipelines[Pipeline_BloomDownsample].Layout,
                Renderer->Pipelines[Pipeline_BloomDownsample].Pipeline,
                Renderer->Pipelines[Pipeline_BloomUpsample].Layout,
                Renderer->Pipelines[Pipeline_BloomUpsample].Pipeline,
                Renderer->SetLayouts[SetLayout_Bloom],
                Renderer->SetLayouts[SetLayout_BloomUpsample]);

    // Blit + UI
    {
        VkImageMemoryBarrier2 BeginBarriers[] = 
        {
            // Swapchain image
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Frame->SwapchainImage,
                .subresourceRange = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
        };

        VkDependencyInfo BeginDependencyInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = CountOf(BeginBarriers),
            .pImageMemoryBarriers = BeginBarriers,
        };
        vkCmdPipelineBarrier2(Frame->CmdBuffer, &BeginDependencyInfo); 

        VkRenderingAttachmentInfo ColorAttachment = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Frame->SwapchainImageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = { { 0.0f, 0.0f, 0.0f, 0.0f } } },
        };

        VkRenderingAttachmentInfo DepthAttachment = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Frame->DepthBuffer->View,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .depthStencil = { 1.0f, 0 } },
        };

        VkRenderingInfo RenderingInfo = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = { { 0, 0 }, Frame->RenderExtent },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &ColorAttachment,
            .pDepthAttachment = &DepthAttachment,
            .pStencilAttachment = nullptr,
        };
        vkCmdBeginRendering(Frame->CmdBuffer, &RenderingInfo);
        
        // Blit
        {
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Blit];
            vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdPushConstants(Frame->CmdBuffer, Pipeline.Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 
                               0, sizeof(f32), &GameState->PostProcessParams.Bloom.Strength);
#if 1
            VkDescriptorSet BlitDescriptorSet = VK_NULL_HANDLE;
            VkDescriptorSetAllocateInfo BlitSetInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = Frame->DescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &Renderer->SetLayouts[SetLayout_Blit],
            };
            VkResult Result = vkAllocateDescriptorSets(VK.Device, &BlitSetInfo, &BlitDescriptorSet);
            Assert(Result == VK_SUCCESS);

            VkDescriptorImageInfo ImageInfos[2] = 
            {
                {
                    .sampler = VK_NULL_HANDLE,
                    .imageView = Frame->HDRRenderTargets[0]->MipViews[0],
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
                {
                    .sampler = VK_NULL_HANDLE,
                    .imageView = Frame->HDRRenderTargets[1]->MipViews[0],
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
            };
            VkWriteDescriptorSet SetWrites[2] = 
            {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = BlitDescriptorSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = ImageInfos + 0,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = BlitDescriptorSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = ImageInfos + 1,
                },
            };
            vkUpdateDescriptorSets(VK.Device, CountOf(SetWrites), SetWrites, 0, nullptr);
#else
            VkDescriptorSet BlitDescriptorSet = PushImageDescriptor(Frame,
                                                                    Renderer->SetLayouts[SetLayout_SampledRenderTargetNormalized_PS_CS],
                                                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                    Frame->HDRRenderTargets[0]->View,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
#endif

            vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout,
                                    0, 1, &BlitDescriptorSet, 0, nullptr);

            vkCmdDraw(Frame->CmdBuffer, 3, 1, 0, 0);
        }

        // Gizmos
        {
            geometry_buffer_allocation ArrowMesh = Assets->Meshes[GameState->Editor.GizmoMeshID];
            u32 IndexCount = (u32)(ArrowMesh.IndexBlock->ByteSize / sizeof(vert_index));
            u32 IndexOffset = (u32)(ArrowMesh.IndexBlock->ByteOffset / sizeof(vert_index));
            u32 VertexOffset = (u32)(ArrowMesh.VertexBlock->ByteOffset / sizeof(vertex));

            pipeline_with_layout GizmoPipeline = Renderer->Pipelines[Pipeline_Gizmo];

            auto RenderGizmo = 
                [Renderer, GizmoPipeline,
                CmdBuffer = Frame->CmdBuffer,
                IndexCount, IndexOffset, VertexOffset,
                &Editor = GameState->Editor]
                (m4 BaseTransform, f32 ScaleFactor)
            {
                m4 Scale = M4(ScaleFactor, 0.0f, 0.0f, 0.0f,
                              0.0f, ScaleFactor, 0.0f, 0.0f,
                              0.0f, 0.0f, ScaleFactor, 0.0f,
                              0.0f, 0.0f, 0.0f, 1.0f);
                    
                rgba8 Colors[3] =
                {
                    PackRGBA8(0xFF, 0x00, 0x00),
                    PackRGBA8(0x00, 0xFF, 0x00),
                    PackRGBA8(0x00, 0x00, 0xFF),
                };

                m4 Transforms[3] = 
                {
                    M4(0.0f, 0.0f, 1.0f, 0.0f,
                       1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 0.0f, 1.0f),
                    M4(1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, -1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 0.0f, 1.0f),
                    M4(1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 0.0f, 1.0f),
                };

                if (Editor.Gizmo.IsGlobal)
                {
                    v3 P = BaseTransform.P.xyz;
                    BaseTransform = M4(1.0f, 0.0f, 0.0f, P.x,
                                       0.0f, 1.0f, 0.0f, P.y,
                                       0.0f, 0.0f, 1.0f, P.z,
                                       0.0f, 0.0f, 0.0f, 1.0f);
                }
                else
                {
                    for (u32 AxisIndex = 0; AxisIndex < 3; AxisIndex++)
                    {
                        BaseTransform.C[AxisIndex].xyz = NOZ(BaseTransform.C[AxisIndex].xyz);
                    }
                }

                for (u32 i = 0; i < 3; i++)
                {
                    m4 Transform = BaseTransform * Transforms[i] * Scale;
                    vkCmdPushConstants(CmdBuffer, GizmoPipeline.Layout,
                                       VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                                       0, sizeof(Transform), &Transform);
                    rgba8 Color = Colors[i];
                    if (i == Editor.Gizmo.Selection)
                    {
                        Color.R = Max(Color.R, (u8)0x80);
                        Color.G = Max(Color.G, (u8)0x80);
                        Color.B = Max(Color.B, (u8)0x80);
                    }

                    vkCmdPushConstants(CmdBuffer, GizmoPipeline.Layout,
                                       VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                                       sizeof(Transform), sizeof(Color), &Color);

                    vkCmdDrawIndexed(CmdBuffer, IndexCount, 1, IndexOffset, VertexOffset, 0);
                }
            };

            vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Pipeline);
            vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Layout,
                                    0, 1, &Frame->UniformDescriptorSet, 
                                    0, nullptr);
            if (GameState->Editor.SelectedInstanceIndex != INVALID_INDEX_U32)
            {
                mesh_instance* Instance = World->Instances + GameState->Editor.SelectedInstanceIndex;
                RenderGizmo(Instance->Transform, 0.25f);
            }
        }

        pipeline_with_layout UIPipeline = Renderer->Pipelines[Pipeline_UI];
        VkDescriptorSetLayout SetLayout = Frame->Renderer->SetLayouts[SetLayout_SingleCombinedTexturePS];
        VkDescriptorSet FontDescriptorSet = PushImageDescriptor(Frame, SetLayout, 
                                                                Assets->DefaultFontTextureID);
        RenderImmediates(Frame, UIPipeline.Pipeline, UIPipeline.Layout, FontDescriptorSet);

        vkCmdEndRendering(Frame->CmdBuffer);
    }

    VkImageMemoryBarrier2 EndBarriers[] = 
    {
        // Swapchain
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
            .dstAccessMask = VK_ACCESS_2_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Frame->SwapchainImage,
            .subresourceRange = 
            {

                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
    };

    VkDependencyInfo EndDependencyInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = CountOf(EndBarriers),
        .pImageMemoryBarriers = EndBarriers,
    };
    vkCmdPipelineBarrier2(Frame->CmdBuffer, &EndDependencyInfo);
}

internal bool AddNodeToScene(game_world* World, gltf* GLTF, 
                             u32 NodeIndex, m4 Transform, 
                             u32 BaseMeshIndex)
{
    bool Result = true;

    if (NodeIndex < GLTF->NodeCount)
    {
        gltf_node* Node = GLTF->Nodes + NodeIndex;

        m4 NodeTransform = Node->Transform;
        if (Node->IsTRS)
        {
            m4 S = M4(Node->Scale.x, 0.0f, 0.0f, 0.0f,
                      0.0f, Node->Scale.y, 0.0f, 0.0f,
                      0.0f, 0.0f, Node->Scale.z, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f);
            m4 R = QuaternionToM4(Node->Rotation);
            m4 T = M4(1.0f, 0.0f, 0.0f, Node->Translation.x,
                      0.0f, 1.0f, 0.0f, Node->Translation.y,
                      0.0f, 0.0f, 1.0f, Node->Translation.z,
                      0.0f, 0.0f, 0.0f, 1.0f);
            NodeTransform = T * R * S;
        }

        NodeTransform = Transform * NodeTransform;
        if (Node->MeshIndex != U32_MAX)
        {
            if (Node->MeshIndex >= GLTF->MeshCount) return false;

            u32 MeshOffset = 0;
            for (u32 i = 0; i < Node->MeshIndex; i++)
            {
                MeshOffset += GLTF->Meshes[i].PrimitiveCount;
            }

            for (u32 i = 0; i < GLTF->Meshes[Node->MeshIndex].PrimitiveCount; i++)
            {
                if (World->InstanceCount >= World->MaxInstanceCount) return false;
                World->Instances[World->InstanceCount++] = 
                {
                    .MeshID = BaseMeshIndex + (MeshOffset + i),
                    .Transform = NodeTransform,
                };
            }
        }

        for (u32 i = 0; i < Node->ChildrenCount; i++)
        {
            Result = AddNodeToScene(World, GLTF, Node->Children[i], NodeTransform, BaseMeshIndex);
            if (!Result)
            {
                break;
            }
        }
    }

    return Result;
}

struct NvttOutputHandler : public nvtt::OutputHandler
{
    u64 MemorySize;
    u64 MemoryAt;
    u8* Memory;

    NvttOutputHandler(u64 MemorySize, void* Memory) :
        MemorySize(MemorySize),
        MemoryAt(0),
        Memory((u8*)Memory)
    { 
    }

    void beginImage(int Size, int Width, int Height, int Depth, int Face, int Mip) override
    {
    }

    bool writeData(const void* Data, int iSize) override
    {
        bool Result = false;

        u64 Size = (u64)iSize;
        if (MemorySize - MemoryAt >= Size)
        {
            memcpy(Memory + MemoryAt, Data, Size);
            MemoryAt += Size;
            Result = true;
        }
        return Result;
    }

    void endImage() override
    {
    }
};

// TODO(boti): remove the renderer from here
internal void LoadTestScene(memory_arena* Scratch, assets* Assets, game_world* World, renderer* Renderer, const char* ScenePath, m4 BaseTransform)
{
    constexpr u64 PathBuffSize = 256;
    char PathBuff[PathBuffSize];
    u64 PathDirectoryLength = 0;
    for (u64 i = 0; ScenePath[i]; i++)
    {
        if (ScenePath[i] == '/' || ScenePath[i] == '\\')
        {
            PathDirectoryLength = i + 1;
        }
    }
    strncpy(PathBuff, ScenePath, PathDirectoryLength);

    u64 FilenameBuffSize = PathBuffSize - PathDirectoryLength;
    char* Filename = PathBuff + PathDirectoryLength;

    gltf GLTF = {};
    // JSON test parsing
    {
        buffer JSONBuffer = Platform.LoadEntireFile(ScenePath, Scratch);
        if (JSONBuffer.Data)
        {
            json_element* Root = ParseJSON(JSONBuffer.Data, JSONBuffer.Size, Scratch);
            if (Root)
            {
                bool GLTFParseResult = ParseGLTF(&GLTF, Root, Scratch);
                if (!GLTFParseResult)
                {
                    UnhandledError("Couldn't parse glTF file");
                    return;
                }
            }
            else
            {
                UnhandledError("Couldn't parse JSON file");
                return;
            }
        }
        else
        {
            UnhandledError("Couldn't load scene file");
            return;
        }
    }

    // NOTE(boti): Store the initial indices in the game so that we know what to offset the glTF indices by
    u32 BaseMeshIndex = Assets->MeshCount;
    u32 BaseMaterialIndex = Assets->MaterialCount;

    buffer* Buffers = PushArray<buffer>(Scratch, GLTF.BufferCount, MemPush_Clear);
    for (u32 BufferIndex = 0; BufferIndex < GLTF.BufferCount; BufferIndex++)
    {
        string URI = GLTF.Buffers[BufferIndex].URI;
        if (URI.Length <= FilenameBuffSize)
        {
            strncpy(Filename, URI.String, URI.Length);
            Filename[URI.Length] = 0;
            Buffers[BufferIndex] = Platform.LoadEntireFile(PathBuff, Scratch);
            if (!Buffers[BufferIndex].Data)
            {
                UnhandledError("Couldn't load glTF buffer");
            }
        }
        else
        {
            UnhandledError("glTF resource filename too long");
        }
    }

    enum class texture_type : u32
    {
        Diffuse,
        Normal,
        Material,
    };

    auto LoadAndUploadTexture = [&](u32 TextureIndex, texture_type Type, gltf_alpha_mode AlphaMode) -> texture_id
    {
        texture_id Result = { 0 };

        memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Scratch);
        if (TextureIndex < GLTF.TextureCount)
        {
            gltf_texture* Texture = GLTF.Textures + TextureIndex;
            if (Texture->ImageIndex >= GLTF.ImageCount) UnhandledError("Invalid glTF image index");

            gltf_image* Image = GLTF.Images + Texture->ImageIndex;
            if (Image->BufferViewIndex != U32_MAX) UnimplementedCodePath;

            if (Image->URI.Length > FilenameBuffSize) UnhandledError("Invalid glTF image path");

            strncpy(Filename, Image->URI.String, Image->URI.Length);
            Filename[Image->URI.Length] = 0;

            nvtt::Surface Surface;
            bool HasAlpha = false;
            if (Surface.load(PathBuff, &HasAlpha, false, nullptr))
            {
#if 1
                constexpr int ResolutionLimit = 2048;
                while (Surface.width() > ResolutionLimit || Surface.height() > ResolutionLimit)
                {
                    Surface.buildNextMipmap(nvtt::MipmapFilter_Box);
                }
#endif

                int Width = Surface.width();
                int Height = Surface.height();
                int MipCount = Surface.countMipmaps(1);

                nvtt::Context Ctx(true);
                nvtt::CompressionOptions CompressionOptions;
                nvtt::OutputOptions OutputOptions;

                nvtt::Format CompressFormat = nvtt::Format_RGB;
                VkFormat Format = VK_FORMAT_UNDEFINED;
                switch (Type)
                {
                    case texture_type::Diffuse:
                        if (AlphaMode == GLTF_ALPHA_MODE_OPAQUE)
                        {
                            Format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
                            CompressFormat = nvtt::Format_BC1;
                        }
                        else
                        {
                            Format = VK_FORMAT_BC3_SRGB_BLOCK; 
                            CompressFormat = nvtt::Format_BC3;
                        }
                        break;
                    case texture_type::Normal:
                        Format = VK_FORMAT_BC5_UNORM_BLOCK; 
                        CompressFormat = nvtt::Format_BC5;
                        break;
                    case texture_type::Material:
                        Format = VK_FORMAT_BC3_UNORM_BLOCK; 
                        CompressFormat = nvtt::Format_BC3;
                        break;
                }

                CompressionOptions.setFormat(CompressFormat);
                CompressionOptions.setQuality(nvtt::Quality_Fastest);

                u64 Size = (u64)Ctx.estimateSize(Surface, MipCount, CompressionOptions);
                void* Texels = PushSize(Scratch, Size, 64);
                NvttOutputHandler OutputHandler(Size, Texels);
                OutputOptions.setOutputHandler(&OutputHandler);

                for (int Mip = 0; Mip < MipCount; Mip++)
                {
                    bool CompressionResult = Ctx.compress(Surface, 0, Mip, CompressionOptions, OutputOptions);
                    if (!CompressionResult)
                    {
                        UnhandledError("Failed to compress mip level");
                    }

                    if (Mip < MipCount - 1)
                    {
                        Surface.buildNextMipmap(nvtt::MipmapFilter_Box);
                    }
                }

                Result = PushTexture(Renderer, (u32)Width, (u32)Height, (u32)MipCount, Texels, Format, {});
            }
            else
            {
                UnhandledError("Couldn't load image");
            }
        }
        else
        {
            //Assert(!"WARNING: default texture");
        }

        RestoreArena(Scratch, Checkpoint);
        return Result;
    };

    texture_id* TextureTable = PushArray<texture_id>(Scratch, GLTF.TextureCount);
    for (u32 i = 0; i < GLTF.TextureCount; i++)
    {
        TextureTable[i].Value = INVALID_INDEX_U32;
    }

    for (u32 MaterialIndex = 0; MaterialIndex < GLTF.MaterialCount; MaterialIndex++)
    {
        gltf_material* SrcMaterial = GLTF.Materials + MaterialIndex;

        if (SrcMaterial->BaseColorTexture.TexCoordIndex != 0) UnimplementedCodePath;
        if (SrcMaterial->NormalTexture.TexCoordIndex != 0) UnimplementedCodePath;
        if (SrcMaterial->MetallicRoughnessTexture.TexCoordIndex != 0) UnimplementedCodePath;

        if (SrcMaterial->NormalTexture.Scale != 1.0f) UnimplementedCodePath;

        if (Assets->MaterialCount < Assets->MaxMaterialCount)
        {
            material* Material = Assets->Materials + Assets->MaterialCount++;

            Material->Emissive = SrcMaterial->EmissiveFactor;
            Material->DiffuseColor = PackRGBA(SrcMaterial->BaseColorFactor);
            Material->BaseMaterial = PackRGBA(v4{ 1.0f, SrcMaterial->RoughnessFactor, SrcMaterial->MetallicFactor, 1.0f });
            Material->DiffuseID = Assets->DefaultDiffuseID;
            Material->NormalID = Assets->DefaultNormalID;
            Material->MetallicRoughnessID = Assets->DefaultMetallicRoughnessID;

            if (SrcMaterial->BaseColorTexture.TextureIndex != U32_MAX)
            {
                texture_id* Texture = TextureTable + SrcMaterial->BaseColorTexture.TextureIndex;
                if (Texture->Value == INVALID_INDEX_U32)
                {
                    *Texture = LoadAndUploadTexture(SrcMaterial->BaseColorTexture.TextureIndex, texture_type::Diffuse, SrcMaterial->AlphaMode);
                }
                Material->DiffuseID = *Texture;
            }
            if (SrcMaterial->NormalTexture.TextureIndex != U32_MAX)
            {
                texture_id* Texture = TextureTable + SrcMaterial->NormalTexture.TextureIndex;
                if (Texture->Value == INVALID_INDEX_U32)
                {
                    *Texture = LoadAndUploadTexture(SrcMaterial->NormalTexture.TextureIndex, texture_type::Normal, SrcMaterial->AlphaMode);
                }
                Material->NormalID = *Texture;
            }
            if (SrcMaterial->MetallicRoughnessTexture.TextureIndex != U32_MAX)
            {
                texture_id* Texture = TextureTable + SrcMaterial->MetallicRoughnessTexture.TextureIndex;
                if (Texture->Value == INVALID_INDEX_U32)
                {
                    *Texture = LoadAndUploadTexture(SrcMaterial->MetallicRoughnessTexture.TextureIndex, texture_type::Material, SrcMaterial->AlphaMode);
                }
                Material->MetallicRoughnessID = *Texture;
            }
        }
        else
        {
            UnhandledError("Out of material pool");
        }
    }

    for (u32 MeshIndex = 0; MeshIndex < GLTF.MeshCount; MeshIndex++)
    {
        gltf_mesh* Mesh = GLTF.Meshes + MeshIndex;
        for (u32 PrimitiveIndex = 0; PrimitiveIndex < Mesh->PrimitiveCount; PrimitiveIndex++)
        {
            memory_arena_checkpoint Checkpoint = ArenaCheckpoint(Scratch);

            gltf_mesh_primitive* Primitive = Mesh->Primitives + PrimitiveIndex;
            if (Primitive->Topology != GLTF_TRIANGLES)
            {
                UnimplementedCodePath;
            }
            
            gltf_accessor* PAccessor = (Primitive->PositionIndex < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->PositionIndex : nullptr;
            gltf_accessor* NAccessor = (Primitive->NormalIndex < GLTF.AccessorCount)  ? GLTF.Accessors + Primitive->NormalIndex : nullptr;
            gltf_accessor* TAccessor = (Primitive->TangentIndex < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->TangentIndex : nullptr;
            gltf_accessor* TCAccessor = (Primitive->TexCoordIndex[0] < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->TexCoordIndex[0] : nullptr;

            gltf_iterator ItP   = MakeGLTFAttribIterator(&GLTF, PAccessor, Buffers);
            gltf_iterator ItN   = MakeGLTFAttribIterator(&GLTF, NAccessor, Buffers);
            gltf_iterator ItT   = MakeGLTFAttribIterator(&GLTF, TAccessor, Buffers);
            gltf_iterator ItTC  = MakeGLTFAttribIterator(&GLTF, TCAccessor, Buffers);

            u32 VertexCount = (u32)ItP.Count;
            if (VertexCount == 0)
            {
                UnhandledError("Missing glTF position data");
            }

            if ((ItN.Count && (ItN.Count != VertexCount)) ||
                (ItT.Count && (ItT.Count!= VertexCount)) ||
                (ItTC.Count && (ItTC.Count!= VertexCount)))
            {
                UnhandledError("Inconsistent glTF vertex attribute counts");
            }

            if (!PAccessor)
            {
                UnhandledError("glTF No position data for mesh");
            }

            if (PAccessor->Type != GLTF_VEC3)
            {
                UnhandledError("Invalid glTF Position type");
            }

            mmbox Box = {};
            for (u32 i = 0; i < 3; i++)
            {
                Box.Min.E[i] = PAccessor->Min.EE[i];
                Box.Max.E[i] = PAccessor->Max.EE[i];
            }

            vertex* VertexData = PushArray<vertex>(Scratch, VertexCount);
            for (u32 i = 0; i < VertexCount; i++)
            {
                vertex* V = VertexData + i;
                V->P = ItP.Get<v3>();
                V->N = ItN.Get<v3>();
                V->T = ItT.Get<v4>();
                V->TexCoord = ItTC.Get<v2>();

                ++ItP;
                ++ItN;
                ++ItT;
                ++ItTC;
            }

            gltf_accessor* IndexAccessor = (Primitive->IndexBufferIndex < GLTF.AccessorCount) ? GLTF.Accessors + Primitive->IndexBufferIndex : nullptr;
            gltf_iterator ItIndex = MakeGLTFAttribIterator(&GLTF, IndexAccessor, Buffers);

            u32 IndexCount = (u32)ItIndex.Count;
            if (IndexCount == 0) UnimplementedCodePath;
            u32* IndexData = PushArray<u32>(Scratch, IndexCount);
            for (u32 i = 0; i < IndexCount; i++)
            {
                if (IndexAccessor->ComponentType == GLTF_USHORT ||
                    IndexAccessor->ComponentType == GLTF_SSHORT)
                {
                    IndexData[i] = ItIndex.Get<u16>();
                }
                else if (IndexAccessor->ComponentType == GLTF_UINT ||
                         IndexAccessor->ComponentType == GLTF_SINT)
                {
                    IndexData[i] = ItIndex.Get<u32>();
                }
                else
                {
                    UnhandledError("Invalid glTF index type");
                }

                ++ItIndex;
            }

            if (ItT.Count == 0)
            {
                v3* Tangents = PushArray<v3>(Scratch, VertexCount, MemPush_Clear);
                v3* Bitangents = PushArray<v3>(Scratch, VertexCount, MemPush_Clear);
                for (u32 i = 0; i < IndexCount; i += 3)
                {
                    u32 Index0 = IndexData[i + 0];
                    u32 Index1 = IndexData[i + 1];
                    u32 Index2 = IndexData[i + 2];
                    vertex* V0 = VertexData + Index0;
                    vertex* V1 = VertexData + Index1;
                    vertex* V2 = VertexData + Index2;

                    v3 dP1 = V1->P - V0->P;
                    v3 dP2 = V2->P - V0->P;

                    v2 dT1 = V1->TexCoord - V0->TexCoord;
                    v2 dT2 = V2->TexCoord - V0->TexCoord;

                    f32 InvDetT = 1.0f / (dT1.x * dT2.y - dT1.y * dT2.x);

                    v3 Tangent = 
                    {
                        (dP1.x * dT2.y - dP2.x * dT1.y) * InvDetT,
                        (dP1.y * dT2.y - dP2.y * dT1.y) * InvDetT,
                        (dP1.z * dT2.y - dP2.z * dT1.y) * InvDetT,
                    };

                    v3 Bitangent = 
                    {
                        (dP1.x * dT2.x - dP2.x * dT1.x) * InvDetT,
                        (dP1.y * dT2.x - dP2.y * dT1.x) * InvDetT,
                        (dP1.z * dT2.x - dP2.z * dT1.x) * InvDetT,
                    };

                    Tangents[Index0] += Tangent;
                    Tangents[Index1] += Tangent;
                    Tangents[Index2] += Tangent;

                    Bitangents[Index0] += Bitangent;
                    Bitangents[Index1] += Bitangent;
                    Bitangents[Index2] += Bitangent;
                }

                for (u32 i = 0; i < VertexCount; i++)
                {
                    vertex* V = VertexData + i;
                    
                    v3 T = Tangents[i] - (V->N * Dot(V->N, Tangents[i]));
                    if (Dot(T, T) > 1e-7f)
                    {
                        T = Normalize(T);
                    }

                    v3 B = Cross(V->N, Tangents[i]);
                    f32 w = Dot(B, Bitangents[i]) < 0.0f ? -1.0f : 1.0f;
                    V->T = { T.x, T.y, T.z, w };
                }
            }

            if (Assets->MeshCount < Assets->MaxMeshCount)
            {
                u32 MeshID = Assets->MeshCount++;
                Assets->Meshes[MeshID] = UploadVertexData(Renderer,
                                                          VertexCount, VertexData,
                                                          IndexCount, IndexData);
                Assets->MeshBoxes[MeshID] = Box;
                Assets->MeshMaterialIndices[MeshID] = Primitive->MaterialIndex + BaseMaterialIndex;
            }
            else
            {
                UnhandledError("Out of mesh pool memory");
            }

            RestoreArena(Scratch, Checkpoint);
        }
    }

    if (GLTF.DefaultSceneIndex >= GLTF.SceneCount)
    {
        UnhandledError("glTF default scene index out of bounds");
    }

    gltf_scene* Scene = GLTF.Scenes + GLTF.DefaultSceneIndex;
    for (u32 RootNodeIt = 0; RootNodeIt < Scene->RootNodeCount; RootNodeIt++)
    {
        AddNodeToScene(World, &GLTF, Scene->RootNodes[RootNodeIt], BaseTransform, BaseMeshIndex);
    }
}

vulkan VK;
platform_api Platform;

extern "C"
void Game_UpdateAndRender(game_memory* Memory, game_io* GameIO)
{
    Platform = Memory->PlatformAPI;
    if (GameIO->bQuitRequested)
    {
        // NOTE(boti): This is where handling game quit should be happening (saving the game, cleaning up if needed, etc.)
        return;
    }

    game_state* GameState = Memory->GameState;
    if (!GameState)
    {
        memory_arena BootstrapArena = InitializeArena(Memory->Size, Memory->Memory);

        GameState = Memory->GameState = PushStruct<game_state>(&BootstrapArena);
        GameState->TotalArena = BootstrapArena;
        constexpr size_t TransientArenaSize = GiB(1);
        GameState->TransientArena = InitializeArena(TransientArenaSize, PushSize(&GameState->TotalArena, TransientArenaSize, 64));

        if (InitializeVulkan(&GameState->Vulkan) == VK_SUCCESS)
        {
            VK = GameState->Vulkan;
        }
        else
        {
            UnhandledError("Vulkan initialization failed");
            GameIO->bQuitRequested = true;
        }
        
        GameState->Renderer = PushStruct<renderer>(&GameState->TotalArena);
        if (CreateRenderer(GameState->Renderer, &GameState->TotalArena, &GameState->TransientArena) != VK_SUCCESS)
        {
            UnhandledError("Renderer initialization failed");
            GameIO->bQuitRequested = true;
        }

        assets* Assets = GameState->Assets = PushStruct<assets>(&GameState->TotalArena);

        // Default textures
        {
            {
                u32 Value = 0xFFFFFFFFu;
                texture_id Whiteness = PushTexture(GameState->Renderer, 1, 1, 1, &Value, VK_FORMAT_R8G8B8A8_SRGB, {});

                Assets->DefaultDiffuseID = Whiteness;
                Assets->DefaultMetallicRoughnessID = Whiteness;
            }
            {
                u16 Value = 0x8080u;
                Assets->DefaultNormalID = PushTexture(GameState->Renderer, 1, 1, 1, &Value, VK_FORMAT_R8G8_UNORM, {});
            }
        }

        LoadDebugFont(GameState, "data/liberation-mono.lbfnt");

        game_world* World = GameState->World = PushStruct<game_world>(&GameState->TotalArena);

        InitEditor(GameState, &GameState->TransientArena);

        GameState->PostProcessParams.SSAO.Intensity = ssao_params::DefaultIntensity;
        GameState->PostProcessParams.SSAO.MaxDistance = ssao_params::DefaultMaxDistance;
        GameState->PostProcessParams.SSAO.TangentTau = ssao_params::DefaultTangentTau;
        GameState->PostProcessParams.Bloom.FilterRadius = bloom_params::DefaultFilterRadius;
        GameState->PostProcessParams.Bloom.InternalStrength = bloom_params::DefaultInternalStrength;
        GameState->PostProcessParams.Bloom.Strength = bloom_params::DefaultStrength;

        World->Camera.P = { 0.0f, 0.0f, 0.0f };
        World->Camera.FieldOfView = ToRadians(80.0f);
        World->Camera.NearZ = 0.05f;
        World->Camera.FarZ = 50.0f;//1000.0f;
        World->Camera.Yaw = 0.5f * Pi;
    }

    VK = GameState->Vulkan;

    ResetArena(&GameState->TransientArena);

    if (GameIO->bIsMinimized)
    {
        return;
    }

    render_frame* RenderFrame = BeginRenderFrame(GameState->Renderer, GameIO->OutputWidth, GameIO->OutputHeight);

    // Update
    {
        if (GameIO->bHasDroppedFile)
        {
            m4 YUpToZUp = M4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
            LoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, GameIO->DroppedFilename, YUpToZUp);
            GameIO->bHasDroppedFile = false;
        }
        else if (GameState->World->InstanceCount == 0) // NOTE(boti): this is kind of a hack
        {
            m4 BaseTransform = M4(1.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, -1.0f, 0.0f,
                                  0.0f, 1.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 0.0f, 1.0f);
#if 0
            BaseTransform = BaseTransform * M4(50.0f, 0.0f, 0.0f, 0.0f,
                                               0.0f, 50.0f, 0.0f, 0.0f,
                                               0.0f, 0.0f, 50.0f, 0.0f,
                                               0.0f, 0.0f, 0.0f, 1.0f);
#endif
            //LoadTestScene(GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Sponza2/NewSponza_Main_Blender_glTF.gltf", BaseTransform);
            LoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Sponza/Sponza.gltf", BaseTransform);
            //LoadTestScene(GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/bathroom/bathroom.gltf", BaseTransform);
            //LoadTestScene(GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Medieval/scene.gltf", BaseTransform);
        }

        if (DoDebugUI(GameState, GameIO, RenderFrame))
        {
            GameIO->Mouse.dP = { 0.0f, 0.0f };
            GameIO->Keys[SC_MouseLeft].TransitionFlags = 0;
        }

        f32 dt = GameIO->dt;

        if (GameIO->Keys[SC_LeftAlt].bIsDown && GameIO->Keys[SC_F4].bIsDown)
        {
            GameIO->bQuitRequested = true;
        }

        UpdateEditor(GameState, GameIO);

        game_world* World = GameState->World;
        World->SunL = 2.5f * v3{ 10.0f, 7.0f, 3.0f }; // Intensity
        World->SunV = Normalize(v3{ -4.0f, 2.5f, 6.0f }); // Direction (towards the sun)

        camera* Camera = &World->Camera;
        v3 MoveDirection = {};
        if (GameIO->Keys[SC_W].bIsDown) { MoveDirection.z += 1.0f; }
        if (GameIO->Keys[SC_S].bIsDown) { MoveDirection.z -= 1.0f; }
        if (GameIO->Keys[SC_D].bIsDown) { MoveDirection.x += 1.0f; }
        if (GameIO->Keys[SC_A].bIsDown) { MoveDirection.x -= 1.0f; }

        if (GameIO->Keys[SC_Left].bIsDown)
        {
            Camera->Yaw += 1e-1f * dt;
        }
        if (GameIO->Keys[SC_Right].bIsDown)
        {
            Camera->Yaw -= 1e-1f * dt;
        }

        if (GameIO->Keys[SC_MouseLeft].bIsDown)
        {
            constexpr f32 MouseTurnSpeed = 1e-2f;
            Camera->Yaw -= -MouseTurnSpeed * GameIO->Mouse.dP.x;
            Camera->Pitch -= MouseTurnSpeed * GameIO->Mouse.dP.y;
        }

        constexpr f32 PitchClamp = 0.5f * Pi - 1e-3f;
        Camera->Pitch = Clamp(Camera->Pitch, -PitchClamp, PitchClamp);

        m4 CameraTransform = GetTransform(Camera);
        v3 Forward = CameraTransform.Z.xyz;
        v3 Right = CameraTransform.X.xyz;

        constexpr f32 MoveSpeed = 1.0f;
        f32 SpeedMul = GameIO->Keys[SC_LeftShift].bIsDown ? 2.5f : 1.0f;
        Camera->P += (Right * MoveDirection.x + Forward * MoveDirection.z) * SpeedMul * MoveSpeed * dt;

        if (GameIO->Keys[SC_Space].bIsDown) { Camera->P.z += SpeedMul * MoveSpeed * dt; }
        if (GameIO->Keys[SC_LeftControl].bIsDown) { Camera->P.z -= SpeedMul * MoveSpeed * dt; }
    }

    GameRender(GameState, RenderFrame);
    EndRenderFrame(RenderFrame);

    GameState->FrameID++;
}