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

struct skinned_cmd
{
    VkDrawIndexedIndirectCommand Indirect;
    m4 Transform;
    material Material;
};

lbfn void RenderMeshes(render_frame* Frame, game_state* GameState,
                       VkPipelineLayout PipelineLayout,
                       u32 SkinnedCommandCount, skinned_cmd* SkinnedCommands,
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

lbfn void RenderMeshes(render_frame* Frame, game_state* GameState,
                       VkPipelineLayout PipelineLayout,
                       u32 SkinnedCommandCount, skinned_cmd* SkinnedCommands,
                       const frustum* Frustum /*= nullptr*/, bool DoCulling /*= false*/)
{
    assets* Assets = GameState->Assets;
    game_world* World = GameState->World;

    const VkDeviceSize ZeroOffset = 0;
    vkCmdBindIndexBuffer(Frame->CmdBuffer, Frame->Renderer->GeometryBuffer.IndexMemory.Buffer, ZeroOffset, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(Frame->CmdBuffer, 0, 1, &Frame->Renderer->GeometryBuffer.VertexMemory.Buffer, &ZeroOffset);
    for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        entity* Entity = World->Entities + EntityIndex;
        // NOTE(boti): skinned entites go through a different path
        if (HasFlag(Entity->Flags, EntityFlag_Mesh) && !HasFlag(Entity->Flags, EntityFlag_Skin))
        {
            geometry_buffer_allocation Allocation = Assets->Meshes[Entity->MeshID];
            mmbox Box = Assets->MeshBoxes[Entity->MeshID];
            u32 MaterialIndex = Assets->MeshMaterialIndices[Entity->MeshID];

            v4 BoxMin = Entity->Transform * v4{ Box.Min.x, Box.Min.y, Box.Min.z, 1.0f };
            v4 BoxMax = Entity->Transform * v4{ Box.Max.x, Box.Max.y, Box.Max.z, 1.0f };

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
                PushConstants.Transform = Entity->Transform;
                PushConstants.Material = Assets->Materials[MaterialIndex];

                vkCmdPushConstants(Frame->CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT, 
                                   0, sizeof(PushConstants), &PushConstants);
                if (Allocation.IndexBlock)
                {
                    u32 IndexCount = (u32)(Allocation.IndexBlock->ByteSize / sizeof(vert_index));
                    u32 IndexOffset = (u32)(Allocation.IndexBlock->ByteOffset / sizeof(vert_index));
                    u32 VertexOffset = (u32)(Allocation.VertexBlock->ByteOffset / sizeof(vertex));
                    vkCmdDrawIndexed(Frame->CmdBuffer, IndexCount, 1, IndexOffset, VertexOffset, 0);
                }
                else
                {
                    u32 VertexCount = (u32)(Allocation.VertexBlock->ByteSize / sizeof(vertex));
                    u32 VertexOffset = (u32)(Allocation.VertexBlock->ByteOffset / sizeof(vertex));
                    vkCmdDraw(Frame->CmdBuffer, VertexCount, 1, VertexOffset, 0);
                }
            }
        }
    }

    vkCmdBindVertexBuffers(Frame->CmdBuffer, 0, 1, &Frame->SkinningBuffer, &ZeroOffset);
    for (u32 CmdIndex = 0; CmdIndex < SkinnedCommandCount; CmdIndex++)
    {
        skinned_cmd* Cmd = SkinnedCommands + CmdIndex;
        push_constants PushConstants = 
        {
            .Transform = Cmd->Transform,
            .Material = Cmd->Material,
        };
        vkCmdPushConstants(Frame->CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(PushConstants), &PushConstants);
        vkCmdDrawIndexed(Frame->CmdBuffer, Cmd->Indirect.indexCount, 1, Cmd->Indirect.firstIndex, Cmd->Indirect.vertexOffset, 0);
    }
}

internal void GameRender(game_state* GameState, game_io* IO, render_frame* Frame)
{
    renderer* Renderer = GameState->Renderer;
    
    game_world* World = GameState->World;
    assets* Assets = GameState->Assets;
    memory_arena* FrameArena = &GameState->TransientArena;

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

    Frame->SunV = World->SunV;
    Frame->Uniforms.SunV = TransformDirection(ViewTransform, World->SunV);
    Frame->Uniforms.SunL = World->SunL;
    light Lights[] = 
    {
        { { -4.95f, -1.15f, 1.20f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
        { { +3.90f, -1.15f, 1.20f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
        { { -4.95f, +1.75f, 1.20f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },
        { { +3.90f, +1.75f, 1.20f, 1.0f }, { 2.0f, 0.8f, 0.2f, 3.0f } },

        { { +8.95f, +3.60f, 1.30f, 1.0f }, { 0.2f, 0.6f, 1.0f, 2.5f } },
        { { +8.95f, -3.20f, 1.30f, 1.0f }, { 0.6f, 0.2f, 1.0f, 2.5f } },
        { { -9.65f, +3.60f, 1.30f, 1.0f }, { 0.4f, 1.0f, 0.4f, 2.5f } },
        { { -9.65f, -3.20f, 1.30f, 1.0f }, { 0.2f, 0.6f, 1.0f, 2.5f } },
    };
    u32 LightCount = CountOf(Lights);

    {
        Frame->Uniforms.LightCount = LightCount;
        memset(Frame->Uniforms.Lights, 0, sizeof(Frame->Uniforms.Lights));

        constexpr f32 LuminanceThreshold = 1e-3f;
        for (u32 LightIndex = 0; LightIndex < LightCount; LightIndex++)
        {
            light* Light = Lights + LightIndex;

            f32 MaxComponent = Max(Max(Light->E.x, Light->E.y), Light->E.z);
            f32 E = MaxComponent * Light->E.w;
            f32 SquareR = E / LuminanceThreshold;
            f32 R = Sqrt(Max(SquareR, 0.0f));

            v3 ViewP = TransformPoint(Frame->Uniforms.ViewTransform, Light->P.xyz);

            v3 BoundingBox[] = 
            {
                ViewP + v3{ -R, -R, -R },
                ViewP + v3{ +R, -R, -R },
                ViewP + v3{ +R, +R, -R },
                ViewP + v3{ -R, +R, -R },

                ViewP + v3{ -R, -R, +R },
                ViewP + v3{ +R, -R, +R },
                ViewP + v3{ +R, +R, +R },
                ViewP + v3{ -R, +R, +R },
            };

            v2 MinP = { +F32_MAX_NORMAL, +F32_MAX_NORMAL };
            v2 MaxP = { -F32_MAX_NORMAL, -F32_MAX_NORMAL };
            //for (u32 )


            Frame->Uniforms.Lights[LightIndex] = 
            {
                Frame->Uniforms.ViewTransform * Light->P,
                Light->E,
            };
        };
    }

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

    // Animation + Skinning
    u32 SkinnedCmdCount = 0;
    skinned_cmd* SkinnedCmdList = PushArray<skinned_cmd>(FrameArena, World->EntityCount);
#if 1
    {
        u32 MaxUBOSize = (u32)VK.MaxConstantBufferByteSize;
        u32 UBOAlignment = (u32)VK.ConstantBufferAlignment;
        u32 JointBufferAlignment = UBOAlignment / sizeof(m4); // Alignment in # of joints

        VkDescriptorSet JointDescriptorSet = 
            PushBufferDescriptor(Frame, 
                                 Renderer->SetLayouts[SetLayout_PoseTransform], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 
                                 Frame->JointBuffer, 0, MaxUBOSize); 

        VkDescriptorSet SkinningBuffersDescriptorSet = PushDescriptorSet(Frame, Renderer->SetLayouts[SetLayout_Skinning]);

        VkDescriptorBufferInfo DescriptorBufferInfos[] = 
        {
            {
                .buffer = Frame->Renderer->GeometryBuffer.VertexMemory.Buffer,
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            },
            {
                .buffer = Frame->SkinningBuffer,
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            },
        };
        VkWriteDescriptorSet DescriptorWrites[] = 
        {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = SkinningBuffersDescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = DescriptorBufferInfos + 0,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = SkinningBuffersDescriptorSet,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = DescriptorBufferInfos + 1,
            },
        };
        vkUpdateDescriptorSets(VK.Device, CountOf(DescriptorWrites), DescriptorWrites, 0, nullptr);

        pipeline_with_layout SkinningPipeline = Renderer->Pipelines[Pipeline_Skinning];
        vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Pipeline);
        vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Layout,
                                0, 1, &SkinningBuffersDescriptorSet,
                                0, nullptr);

        VkBufferMemoryBarrier2 BeginBarriers[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Frame->SkinningBuffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
        };

        VkDependencyInfo BeginDependencies = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = CountOf(BeginBarriers),
            .pBufferMemoryBarriers = BeginBarriers,
        };
        vkCmdPipelineBarrier2(Frame->CmdBuffer, &BeginDependencies);

        u32 JointOffset = 0;
        for (u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            entity* Entity = World->Entities + EntityIndex;
            if (!HasAllFlags(Entity->Flags, EntityFlag_Mesh|EntityFlag_Skin))
            {
                continue;
            }
            if (Entity->CurrentAnimationID == U32_MAX) continue;

            m4 Transform = Entity->Transform;

            skin* Skin = Assets->Skins + Entity->SkinID;
            animation* Animation = Assets->Animations + Entity->CurrentAnimationID;
            Assert(Animation->SkinID == Entity->SkinID);

            if (Entity->DoAnimation)
            {
                f32 LastKeyFrameTimestamp = Animation->KeyFrameTimestamps[Animation->KeyFrameCount - 1];
                Entity->AnimationCounter += IO->dt;
                // TODO(boti): modulo
                while (Entity->AnimationCounter >= LastKeyFrameTimestamp)
                {
                    Entity->AnimationCounter -= LastKeyFrameTimestamp;
                }
            }
            
            u32 NextKeyFrameIndex;
            for (NextKeyFrameIndex = 0; NextKeyFrameIndex < Animation->KeyFrameCount; NextKeyFrameIndex++)
            {
                if (Entity->AnimationCounter < Animation->KeyFrameTimestamps[NextKeyFrameIndex])
                {
                    break;
                }
            }

            u32 KeyFrameIndex = NextKeyFrameIndex - 1;
            f32 KeyFrameDelta = Animation->KeyFrameTimestamps[NextKeyFrameIndex] - Animation->KeyFrameTimestamps[KeyFrameIndex];
            f32 BlendStart = Entity->AnimationCounter - Animation->KeyFrameTimestamps[KeyFrameIndex];
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

                m4 JointTransform = TRSToM4(Transform);
                u32 ParentIndex = Skin->JointParents[JointIndex];
                if (ParentIndex != U32_MAX)
                {
                    Pose[JointIndex] = Pose[ParentIndex] * JointTransform;
                }
                else
                {
                    Pose[JointIndex] = JointTransform;
                }
            }

            // NOTE(boti): This _cannot_ be folded into the above loop, because the parent transforms must not contain
            // the inverse bind transform when propagating the transforms down the hierarchy
            for (u32 JointIndex = 0; JointIndex < Skin->JointCount; JointIndex++)
            {
                Pose[JointIndex] = Pose[JointIndex] * Skin->InverseBindMatrices[JointIndex];
            }
            memcpy(Frame->JointMapping + JointOffset, Pose, Skin->JointCount * sizeof(m4));
            u32 JointBufferOffset = JointOffset * sizeof(m4);
            JointOffset = Align(JointOffset + Skin->JointCount, JointBufferAlignment);

            geometry_buffer_allocation* Mesh = Assets->Meshes + Entity->MeshID;
            u32 IndexCount = TruncateU64ToU32(Mesh->IndexBlock->ByteSize / sizeof(vert_index));
            u32 IndexOffset = TruncateU64ToU32(Mesh->IndexBlock->ByteOffset / sizeof(vert_index));
            u32 VertexOffset = TruncateU64ToU32(Mesh->VertexBlock->ByteOffset / sizeof(vertex));
            u32 VertexCount = TruncateU64ToU32(Mesh->VertexBlock->ByteSize / sizeof(vertex));
            u32 SkinningVertexOffset = Frame->SkinningBufferOffset;
            Frame->SkinningBufferOffset += VertexCount;

            vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Layout,
                                    1, 1, &JointDescriptorSet,
                                    1, &JointBufferOffset);

            u32 PushConstants[3] = 
            {
                VertexOffset,
                SkinningVertexOffset,
                VertexCount,
            };
            vkCmdPushConstants(Frame->CmdBuffer, SkinningPipeline.Layout, VK_SHADER_STAGE_ALL,
                               0, sizeof(PushConstants), PushConstants);


            constexpr u32 SkinningGroupSize = 64;
            vkCmdDispatch(Frame->CmdBuffer, CeilDiv(VertexCount, SkinningGroupSize), 1, 1);

            u32 MaterialID = Assets->MeshMaterialIndices[Entity->MeshID];
            material Material = Assets->Materials[MaterialID];

            SkinnedCmdList[SkinnedCmdCount++] = 
            {
                .Indirect = 
                {
                    .indexCount = IndexCount,
                    .instanceCount = 1,
                    .firstIndex = IndexOffset,
                    .vertexOffset = (s32)SkinningVertexOffset,
                    .firstInstance = 0,
                },
                .Transform = Entity->Transform,
                .Material = Material,
            };
        }

        VkBufferMemoryBarrier2 EndBarriers[] =
        {
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = Frame->SkinningBuffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            },
        };

        VkDependencyInfo EndDependencies = 
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .bufferMemoryBarrierCount = CountOf(EndBarriers),
            .pBufferMemoryBarriers = EndBarriers,
        };
        vkCmdPipelineBarrier2(Frame->CmdBuffer, &EndDependencies);
    }
#endif

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
            RenderMeshes(Frame, GameState, Pipeline.Layout, SkinnedCmdCount, SkinnedCmdList);

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
            RenderMeshes(Frame, GameState, Pipeline.Layout,
                         SkinnedCmdCount, SkinnedCmdList,
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

            RenderMeshes(Frame, GameState, Pipeline.Layout,
                         SkinnedCmdCount, SkinnedCmdList,
                         &Frame->CameraFrustum, EnablePrimaryCull);

            // Render sky
#if 1
            {
                pipeline_with_layout SkyPipeline = Renderer->Pipelines[Pipeline_Sky];
                vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Pipeline);
                vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Layout, 
                                        0, 1, &Frame->UniformDescriptorSet, 
                                        0, nullptr);
                vkCmdDraw(Frame->CmdBuffer, 3, 1, 0, 0);
            }
#endif

            // Particles
            {
                Frame->Particles[Frame->ParticleCount++] = 
                { 
                    { 0.0f, 0.0f, 0.5f },
                    { 0.5f, 0.5f },
                    5.0f * v3{ 0.8f, 1.0f, 0.4f },
                };

                VkImageView ParticleView = GetImageView(&Renderer->TextureManager, Assets->ParticleArrayID);
                VkDescriptorSet TextureSet = PushDescriptorSet(Frame, Renderer->SetLayouts[SetLayout_SingleCombinedTexturePS]);
                VkDescriptorImageInfo DescriptorImage = 
                {
                    .sampler = Renderer->Samplers[Sampler_Default],
                    .imageView = ParticleView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
                VkWriteDescriptorSet TextureWrite = 
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = TextureSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &DescriptorImage,
                };
                vkUpdateDescriptorSets(VK.Device, 1, &TextureWrite, 0, nullptr);

                VkDescriptorSet ParticleBufferDescriptor = 
                    PushBufferDescriptor(Frame, Renderer->SetLayouts[SetLayout_ParticleBuffer], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
                                         Frame->ParticleBuffer, 0, VK_WHOLE_SIZE);
                VkDescriptorSet ParticleDescriptorSets[] = 
                {
                    Frame->UniformDescriptorSet,
                    ParticleBufferDescriptor,
                    TextureSet,
                };

                pipeline_with_layout ParticlePipeline = Renderer->Pipelines[Pipeline_Quad];
                vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ParticlePipeline.Pipeline);
                vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ParticlePipeline.Layout,
                                        0, CountOf(ParticleDescriptorSets), ParticleDescriptorSets, 
                                        0, nullptr);
                vkCmdDraw(Frame->CmdBuffer, 6 * Frame->ParticleCount, 1, 0, 0);
            }
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

        // 3D debug render
        const VkDeviceSize ZeroOffset = 0;
        vkCmdBindVertexBuffers(Frame->CmdBuffer, 0, 1, &Renderer->GeometryBuffer.VertexMemory.Buffer, &ZeroOffset);
        vkCmdBindIndexBuffer(Frame->CmdBuffer, Renderer->GeometryBuffer.IndexMemory.Buffer, ZeroOffset, VK_INDEX_TYPE_UINT32);

        pipeline_with_layout GizmoPipeline = Renderer->Pipelines[Pipeline_Gizmo];
        vkCmdBindPipeline(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Pipeline);
        vkCmdBindDescriptorSets(Frame->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Layout,
                                0, 1, &Frame->UniformDescriptorSet, 
                                0, nullptr);

        if (GameState->Editor.DrawLights)
        {
            geometry_buffer_allocation SphereMesh = Assets->Meshes[Assets->SphereMeshID];
            u32 IndexCount = (u32)SphereMesh.IndexBlock->ByteSize / sizeof(vert_index);
            u32 IndexOffset = (u32)SphereMesh.IndexBlock->ByteOffset / sizeof(vert_index);
            u32 VertexOffset = (u32)SphereMesh.VertexBlock->ByteOffset / sizeof(vertex);

            for (u32 LightIndex = 0; LightIndex < LightCount; LightIndex++)
            {
                light* Light = Lights + LightIndex;
                m4 Transform = M4(1e-1f, 0.0f, 0.0f, Light->P.x,
                                  0.0f, 1e-1f, 0.0f, Light->P.y,
                                  0.0f, 0.0f, 1e-1f, Light->P.z,
                                  0.0f, 0.0f, 0.0f, 1.0f);
                rgba8 Color = PackRGBA({ Light->E.x, Light->E.y, Light->E.z, 1.0f });

                vkCmdPushConstants(Frame->CmdBuffer, GizmoPipeline.Layout,
                                   VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0, sizeof(Transform), &Transform);
                vkCmdPushConstants(Frame->CmdBuffer, GizmoPipeline.Layout,
                                   VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                                   sizeof(Transform), sizeof(Color), &Color);
                vkCmdDrawIndexed(Frame->CmdBuffer, IndexCount, 1, IndexOffset, VertexOffset, 0);
            }
        }

        if (GameState->Editor.IsEnabled)
        {
            geometry_buffer_allocation ArrowMesh = Assets->Meshes[GameState->Editor.GizmoMeshID];
            u32 IndexCount = (u32)(ArrowMesh.IndexBlock->ByteSize / sizeof(vert_index));
            u32 IndexOffset = (u32)(ArrowMesh.IndexBlock->ByteOffset / sizeof(vert_index));
            u32 VertexOffset = (u32)(ArrowMesh.VertexBlock->ByteOffset / sizeof(vertex));

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

            if (IsValid(GameState->Editor.SelectedEntityID))
            {
                entity* Entity = World->Entities + GameState->Editor.SelectedEntityID.Value;
                RenderGizmo(Entity->Transform, 0.25f);
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
        Platform.DebugPrint("sizeof(vertex) = %d\n", sizeof(vertex));
        Platform.DebugPrint("\t.P  = %d\n", OffsetOf(vertex, P));
        Platform.DebugPrint("\t.N  = %d\n", OffsetOf(vertex, N));
        Platform.DebugPrint("\t.T  = %d\n", OffsetOf(vertex, T));
        Platform.DebugPrint("\t.TC = %d\n", OffsetOf(vertex, TexCoord));
        Platform.DebugPrint("\t.W  = %d\n", OffsetOf(vertex, Weights));
        Platform.DebugPrint("\t.J  = %d\n", OffsetOf(vertex, Joints));
        Platform.DebugPrint("\t.C  = %d\n", OffsetOf(vertex, Color));
        Platform.DebugPrint("sizeof(vertex_skin8) = %d\n", sizeof(vertex_skin8));

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
                texture_id Whiteness = PushTexture(GameState->Renderer, TextureFlag_None, 1, 1, 1, 1, VK_FORMAT_R8G8B8A8_SRGB, {}, &Value);

                Assets->DefaultDiffuseID = Whiteness;
                Assets->DefaultMetallicRoughnessID = Whiteness;
            }
            {
                u16 Value = 0x8080u;
                Assets->DefaultNormalID = PushTexture(GameState->Renderer, TextureFlag_None, 1, 1, 1, 1, VK_FORMAT_R8G8_UNORM, {}, &Value);
            }
        }

        // Particle textures
        {
            // TODO(boti): For now we know that the texture pack we're using is 512x512, 
            // but we may want to figure out some way for the user code to pack texture arrays/atlases dynamically
            u32 ParticleWidth = 512;
            u32 ParticleHeight = 512;
            u32 ParticleCount = Particle_COUNT;

            umm ImageSize = ParticleWidth * ParticleHeight;
            umm MemorySize = ImageSize * ParticleCount;
            void* Memory = PushSize(&GameState->TransientArena, MemorySize, 0x100);

            u8* MemoryAt = (u8*)Memory;
            for (u32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
            {
                s32 Width, Height, ChannelCount;
                u8* Texels = stbi_load(ParticlePaths[ParticleIndex], &Width, &Height, &ChannelCount, 1);
                if (Texels)
                {
                    Assert(((u32)Width == ParticleWidth) && ((u32)Height == ParticleHeight));
                    memcpy(MemoryAt, Texels, ImageSize);
                }
                else
                {
                    UnhandledError("Failed to load particle texture");
                }
                stbi_image_free(Texels);
                MemoryAt += ImageSize;
            }

            Assets->ParticleArrayID = PushTexture(
                GameState->Renderer, TextureFlag_Special,
                ParticleWidth, ParticleHeight, 1, ParticleCount, 
                VK_FORMAT_R8_UNORM, { Swizzle_One, Swizzle_One, Swizzle_One, Swizzle_R },
                Memory);
            if (!IsValid(Assets->ParticleArrayID))
            {
                UnhandledError("Failed to create particles texture");
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

    if (!GameState->World->IsLoaded)
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
#elif 0
        BaseTransform = BaseTransform * M4(1e-2f, 0.0f, 0.0f, 0.0f,
                                           0.0f, 1e-2f, 0.0f, 0.0f,
                                           0.0f, 0.0f, 1e-2f, 0.0f,
                                           0.0f, 0.0f, 0.0f, 1.0f);
#endif
        m4 Transform = BaseTransform;
        //DEBUGLoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Sponza2/NewSponza_Main_Blender_glTF.gltf", BaseTransform);
        //DEBUGLoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Medieval/scene.gltf", BaseTransform);
        DEBUGLoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Sponza/Sponza.gltf", Transform);
        //DEBUGLoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/bathroom/bathroom.gltf", Transform);
        Transform = BaseTransform * M4(1e-2f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 1e-2f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1e-2f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f);
        DEBUGLoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, "data/Scenes/Fox/Fox.gltf", Transform);
        GameState->World->IsLoaded = true;
    }

    // Update
    {
        if (GameIO->bHasDroppedFile)
        {
            m4 YUpToZUp = M4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
#if 1
            m4 Transform = YUpToZUp;
#else
            m4 Transform = M4(1e-2f, 0.0f, 0.0f, 5.0f,
                              0.0f, 1e-2f, 0.0f, 0.0f,
                              0.0f, 0.0f, 1e-2f, 0.0f,
                              0.0f, 0.0f, 0.0f, 1.0f) * YUpToZUp;
#endif
            DEBUGLoadTestScene(&GameState->TransientArena, GameState->Assets, GameState->World, GameState->Renderer, GameIO->DroppedFilename, Transform);
            GameIO->bHasDroppedFile = false;
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
        World->SunV = Normalize(v3{ -3.0f, 2.5f, 12.0f }); // Direction (towards the sun)

        if (IsValid(GameState->Editor.SelectedEntityID))
        {
            entity* Entity = World->Entities + GameState->Editor.SelectedEntityID.Value;
            if (HasFlag(Entity->Flags, EntityFlag_Skin))
            {
                if (WasPressed(GameIO->Keys[SC_P]))
                {
                    Entity->DoAnimation = !Entity->DoAnimation;
                }

                if (WasPressed(GameIO->Keys[SC_0]))
                {
                    Entity->CurrentAnimationID = 0;
                    Entity->AnimationCounter = 0.0f;
                }
                for (u32 Scancode = SC_1; Scancode <= SC_9; Scancode++)
                {
                    if (WasPressed(GameIO->Keys[Scancode]))
                    {
                        u32 Index = Scancode - SC_1 + 1;
                        if (Index < GameState->Assets->AnimationCount)
                        {
                            Entity->CurrentAnimationID = Index;
                            Entity->AnimationCounter = 0.0f;
                        }
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