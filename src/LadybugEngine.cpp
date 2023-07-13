#include "LadybugEngine.hpp"

#include "LadybugLib/LBLibBuild.cpp"
//#include "Renderer/Renderer.cpp"

#include "Debug.cpp"
#include "Font.cpp"
#include "Asset.cpp"
#include "World.cpp"
#include "Editor.cpp"

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable:4244)
#include "stb/stb_image.h"
#pragma warning(pop)

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

internal void GameRender(game_state* GameState, game_io* IO, render_frame* Frame)
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

            // Skinned mesh rendering
            {
                VkDescriptorSet JointDescriptorSet = 
                    PushBufferDescriptor(Frame, 
                        Renderer->SetLayouts[SetLayout_Skinned], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 
                        Frame->JointBuffer, 0, 1 << 16); // TODO(boti): The size here should be the UBO size from the device

                pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Skinned];
                vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
                vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout,
                                        0, 1, &Frame->UniformDescriptorSet,
                                        0, nullptr);

                u32 JointOffset = 0;
                for (u32 Index = 0; Index < World->SkinnedInstanceCount; Index++)
                {
                    skinned_mesh_instance* Instance = World->SkinnedInstances + Index;
                    m4 Transform = Instance->Transform;
                    skin* Skin = Assets->Skins + Instance->SkinID;

                    animation* Animation = Assets->Animations + Instance->CurrentAnimationID;

                    if (Instance->DoAnimation)
                    {
                        f32 LastKeyFrameTimestamp = Animation->KeyFrameTimestamps[Animation->KeyFrameCount - 1];
                        Instance->AnimationCounter += IO->dt;
                        while (Instance->AnimationCounter >= LastKeyFrameTimestamp)
                        {
                            Instance->AnimationCounter -= LastKeyFrameTimestamp;
                        }
                    }

                    u32 NextKeyFrameIndex;
                    for (NextKeyFrameIndex = 0; NextKeyFrameIndex < Animation->KeyFrameCount; NextKeyFrameIndex++)
                    {
                        if (Instance->AnimationCounter < Animation->KeyFrameTimestamps[NextKeyFrameIndex])
                        {
                            break;
                        }
                    }

                    u32 KeyFrameIndex = NextKeyFrameIndex - 1;
                    f32 KeyFrameDelta = Animation->KeyFrameTimestamps[NextKeyFrameIndex] - Animation->KeyFrameTimestamps[KeyFrameIndex];
                    f32 BlendStart = Instance->AnimationCounter - Animation->KeyFrameTimestamps[KeyFrameIndex];
                    f32 BlendFactor = Ratio0(BlendStart, KeyFrameDelta);

                    animation_key_frame* CurrentFrame = Animation->KeyFrames + KeyFrameIndex;
                    animation_key_frame* NextFrame = Animation->KeyFrames + NextKeyFrameIndex;

                    m4 Pose[skin::MaxJointCount] = {};
                    for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                    {
                        trs_transform* CurrentTransform = CurrentFrame->JointTransforms + JointIndex;
                        trs_transform* NextTransform = NextFrame->JointTransforms + JointIndex;
                        trs_transform Transform;
                        Transform.Rotation = Normalize(Lerp(CurrentTransform->Rotation, NextTransform->Rotation, BlendFactor));
                        Transform.Position = Lerp(CurrentTransform->Position, NextTransform->Position, BlendFactor);
                        Transform.Scale = Lerp(CurrentTransform->Scale, NextTransform->Scale, BlendFactor);

                        m4 JointPose = TRSToM4(Transform);
                        if (Skin->JointParents[JointIndex] != U32_MAX)
                        {
                            Pose[JointIndex] = Pose[Skin->JointParents[JointIndex]] * JointPose;
                        }
                        else
                        {
                            Pose[JointIndex] = JointPose;
                        }
                    }

                    for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
                    {
                        Pose[JointIndex] = Pose[JointIndex] * Skin->InverseBindMatrices[JointIndex];
                    }
                    memcpy(Frame->JointMapping + JointOffset, Pose, Skin->JointCount * sizeof(m4));
                    u32 JointBufferOffset = JointOffset * sizeof(m4);
                    JointOffset += Skin->JointCount;

                    geometry_buffer_allocation* Mesh = Assets->Meshes + Instance->MeshID;
                    u32 IndexCount = Mesh->IndexBlock->ByteSize / sizeof(u32);
                    u32 IndexOffset = Mesh->IndexBlock->ByteOffset / sizeof(u32);
                    u32 VertexOffset = Mesh->VertexBlock->ByteOffset / sizeof(vertex);

                    vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout,
                                            1, 1, &JointDescriptorSet,
                                            1, &JointBufferOffset);

                    u32 MaterialIndex = Assets->MeshMaterialIndices[Instance->MeshID];
                    material Material = Assets->Materials[MaterialIndex];

                    vkCmdPushConstants(Frame->CmdBuffer, Pipeline.Layout, VK_SHADER_STAGE_ALL, 0, sizeof(Transform), &Transform);
                    vkCmdPushConstants(Frame->CmdBuffer, Pipeline.Layout, VK_SHADER_STAGE_ALL, sizeof(Transform), sizeof(Material), &Material);

                    Assert(IndexCount > 0);
                    vkCmdDrawIndexed(Frame->CmdBuffer, IndexCount, 1, IndexOffset, VertexOffset, 0);
                }
            }

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
        Assets->Arena = &GameState->TotalArena;

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

        LoadDebugFont(&GameState->TransientArena, GameState->Assets, GameState->Renderer, "data/liberation-mono.lbfnt");

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
        else if (!GameState->World->IsLoaded)
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
#elif 1
            BaseTransform = BaseTransform * M4(1e-2f, 0.0f, 0.0f, 0.0f,
                                               0.0f, 1e-2f, 0.0f, 0.0f,
                                               0.0f, 0.0f, 1e-2f, 0.0f,
                                               0.0f, 0.0f, 0.0f, 1.0f);
#endif
            //LoadTestScene(GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Sponza2/NewSponza_Main_Blender_glTF.gltf", BaseTransform);
            //LoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Sponza/Sponza.gltf", BaseTransform);
            //LoadTestScene(GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/bathroom/bathroom.gltf", BaseTransform);
            //LoadTestScene(GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Medieval/scene.gltf", BaseTransform);
            //LoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Fox/Fox.gltf", BaseTransform);
            GameState->World->IsLoaded = true;
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

        if (GameState->Assets->AnimationCount && 
            World->SkinnedInstanceCount)
        {
            skinned_mesh_instance* Instance = World->SkinnedInstances + 0;
            if (WasPressed(GameIO->Keys[SC_P]))
            {
                Instance->DoAnimation = !Instance->DoAnimation;
            }

            if (WasPressed(GameIO->Keys[SC_0]))
            {
                Instance->CurrentAnimationID = 0;
                Instance->AnimationCounter = 0.0f;
            }
            for (u32 Scancode = SC_1; Scancode <= SC_9; Scancode++)
            {
                if (WasPressed(GameIO->Keys[Scancode]))
                {
                    u32 Index = Scancode - SC_1 + 1;
                    if (Index < GameState->Assets->AnimationCount)
                    {
                        Instance->CurrentAnimationID = Index;
                        Instance->AnimationCounter = 0.0f;
                    }
                }
            }
        }

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

    GameRender(GameState, GameIO, RenderFrame);
    EndRenderFrame(RenderFrame);

    GameState->FrameID++;
}