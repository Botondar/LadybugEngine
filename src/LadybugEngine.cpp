#include "LadybugEngine.hpp"

#include "LadybugLib/LBLibBuild.cpp"

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

lbfn void RenderMeshes(render_frame* Frame, VkPipelineLayout PipelineLayout)
{
    const VkDeviceSize ZeroOffset = 0;
    vkCmdBindIndexBuffer(Frame->Backend->CmdBuffer, Frame->Renderer->GeometryBuffer.IndexMemory.Buffer, ZeroOffset, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(Frame->Backend->CmdBuffer, 0, 1, &Frame->Renderer->GeometryBuffer.VertexMemory.Buffer, &ZeroOffset);

    for (u32 CmdIndex = 0; CmdIndex < Frame->DrawCmdCount; CmdIndex++)
    {
        draw_cmd* Cmd = Frame->DrawCmds + CmdIndex;
        push_constants PushConstants = 
        {
            .Transform = Cmd->Transform,
            .Material = Cmd->Material,
        };

        vkCmdPushConstants(Frame->Backend->CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(PushConstants), &PushConstants);
        vkCmdDrawIndexed(Frame->Backend->CmdBuffer, Cmd->Base.IndexCount, Cmd->Base.InstanceCount, Cmd->Base.IndexOffset, Cmd->Base.VertexOffset, Cmd->Base.InstanceOffset);

    }

    vkCmdBindVertexBuffers(Frame->Backend->CmdBuffer, 0, 1, &Frame->Backend->SkinnedMeshVB, &ZeroOffset);
    for (u32 CmdIndex = 0; CmdIndex < Frame->SkinnedDrawCmdCount; CmdIndex++)
    {
        draw_cmd* Cmd = Frame->SkinnedDrawCmds + CmdIndex;
        push_constants PushConstants = 
        {
            .Transform = Cmd->Transform,
            .Material = Cmd->Material,
        };
        vkCmdPushConstants(Frame->Backend->CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(PushConstants), &PushConstants);
        vkCmdDrawIndexed(Frame->Backend->CmdBuffer, Cmd->Base.IndexCount, 1, Cmd->Base.IndexOffset, Cmd->Base.VertexOffset, 0);
    }
}

internal void GameRender(game_state* GameState, game_io* IO, render_frame* Frame)
{
    renderer* Renderer = GameState->Renderer;
    
    game_world* World = GameState->World;
    assets* Assets = GameState->Assets;
    memory_arena* FrameArena = &GameState->TransientArena;

    // TODO(boti): this should have happened in UpdateAndRenderWorld
    {
        constexpr f32 LuminanceThreshold = 1e-3f;
        for (u32 LightIndex = 0; LightIndex < Frame->Uniforms.LightCount; LightIndex++)
        {
            light* Light = Frame->Uniforms.Lights + LightIndex;
            Light->P = Frame->Uniforms.ViewTransform * Light->P;

            // TODO(boti): Coarse light culling here
#if 0
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
#endif
        };
    }

    BeginSceneRendering(Frame);

    VkDescriptorSet OcclusionDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_SampledRenderTargetPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->Backend->OcclusionBuffers[1]->View,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); 

    VkDescriptorSet StructureBufferDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_SampledRenderTargetPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->Backend->StructureBuffer->View,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); 

    VkDescriptorSet ShadowDescriptorSet = PushImageDescriptor(
        Frame,
        Renderer->SetLayouts[SetLayout_ShadowPS],
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        Frame->Backend->ShadowMapView,
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


    // Skinning
    {
        VkDescriptorSet JointDescriptorSet = 
            PushBufferDescriptor(Frame, 
                                 Renderer->SetLayouts[SetLayout_PoseTransform], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 
                                 Frame->Backend->JointBuffer, 0, VK.MaxConstantBufferByteSize); 

        VkDescriptorSet SkinningBuffersDescriptorSet = PushDescriptorSet(Frame, Renderer->SetLayouts[SetLayout_Skinning]);

        VkDescriptorBufferInfo DescriptorBufferInfos[] = 
        {
            {
                .buffer = Frame->Renderer->GeometryBuffer.VertexMemory.Buffer,
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            },
            {
                .buffer = Frame->Backend->SkinnedMeshVB,
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
        vkCmdBindPipeline(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Pipeline);
        vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Layout,
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
                .buffer = Frame->Backend->SkinnedMeshVB,
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
        vkCmdPipelineBarrier2(Frame->Backend->CmdBuffer, &BeginDependencies);

        for (u32 SkinningCmdIndex = 0; SkinningCmdIndex < Frame->SkinningCmdCount; SkinningCmdIndex++)
        {
            skinning_cmd* Cmd = Frame->SkinningCmds + SkinningCmdIndex;
            vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, SkinningPipeline.Layout,
                                    1, 1, &JointDescriptorSet,
                                    1, &Cmd->PoseOffset);

            u32 PushConstants[3] = 
            {
                Cmd->SrcVertexOffset,
                Cmd->DstVertexOffset,
                Cmd->VertexCount,
            };
            vkCmdPushConstants(Frame->Backend->CmdBuffer, SkinningPipeline.Layout, VK_SHADER_STAGE_ALL,
                               0, sizeof(PushConstants), PushConstants);


            constexpr u32 SkinningGroupSize = 64;
            vkCmdDispatch(Frame->Backend->CmdBuffer, CeilDiv(Cmd->VertexCount, SkinningGroupSize), 1, 1);
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
                .buffer = Frame->Backend->SkinnedMeshVB,
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
        vkCmdPipelineBarrier2(Frame->Backend->CmdBuffer, &EndDependencies);
    }

    // 3D render
    {
        const VkDescriptorSet DescriptorSets[] = 
        {
            Renderer->TextureManager.DescriptorSets[0], // Samplers
            Renderer->TextureManager.DescriptorSets[1], // Images
            Frame->Backend->UniformDescriptorSet,
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
        vkCmdSetViewport(Frame->Backend->CmdBuffer, 0, 1, &ShadowViewport);
        vkCmdSetScissor(Frame->Backend->CmdBuffer, 0, 1, &ShadowScissor);
        for (u32 Cascade = 0; Cascade < R_MaxShadowCascadeCount; Cascade++)
        {
            BeginCascade(Frame, Cascade);

            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Shadow];
            vkCmdBindPipeline(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdPushConstants(Frame->Backend->CmdBuffer, Pipeline.Layout,
                               VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                               sizeof(push_constants), sizeof(u32), &Cascade);
            vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout,
                                    0, 3, DescriptorSets, 0, nullptr);
            RenderMeshes(Frame, Pipeline.Layout);

            EndCascade(Frame);
        }

        bool EnablePrimaryCull = false;
        vkCmdSetViewport(Frame->Backend->CmdBuffer, 0, 1, &FrameViewport);
        vkCmdSetScissor(Frame->Backend->CmdBuffer, 0, 1, &FrameScissor);
        BeginPrepass(Frame);
        {
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Prepass];
            vkCmdBindPipeline(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout, 
                                    0, 3, DescriptorSets,
                                    0, nullptr);
            RenderMeshes(Frame, Pipeline.Layout);
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

            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Simple];
            vkCmdBindPipeline(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout, 
                                    0, CountOf(DescriptorSets), DescriptorSets,
                                    0, nullptr);

            RenderMeshes(Frame, Pipeline.Layout);

            // Render sky
#if 1
            {
                pipeline_with_layout SkyPipeline = Renderer->Pipelines[Pipeline_Sky];
                vkCmdBindPipeline(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Pipeline);
                vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline.Layout, 
                                        0, 1, &Frame->Backend->UniformDescriptorSet, 
                                        0, nullptr);
                vkCmdDraw(Frame->Backend->CmdBuffer, 3, 1, 0, 0);
            }
#endif

            // Particles
            {
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
                                         Frame->Backend->ParticleBuffer, 0, VK_WHOLE_SIZE);
                VkDescriptorSet ParticleDescriptorSets[] = 
                {
                    Frame->Backend->UniformDescriptorSet,
                    ParticleBufferDescriptor,
                    StructureBufferDescriptorSet,
                    TextureSet,
                };

                pipeline_with_layout ParticlePipeline = Renderer->Pipelines[Pipeline_Quad];
                vkCmdBindPipeline(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ParticlePipeline.Pipeline);
                vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ParticlePipeline.Layout,
                                        0, CountOf(ParticleDescriptorSets), ParticleDescriptorSets, 
                                        0, nullptr);

                for (u32 CmdIndex = 0; CmdIndex < Frame->ParticleDrawCmdCount; CmdIndex++)
                {
                    particle_cmd* Cmd = Frame->ParticleDrawCmds + CmdIndex;
                    u32 VertexCount = 6 * Cmd->ParticleCount;
                    u32 FirstVertex = 6 * Cmd->FirstParticle;
                    vkCmdPushConstants(Frame->Backend->CmdBuffer, ParticlePipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 
                                       0, sizeof(Cmd->Mode), &Cmd->Mode);
                    vkCmdDraw(Frame->Backend->CmdBuffer, VertexCount, 1, FirstVertex, 0);
                }
            }
        }
        EndForwardPass(Frame);
    }
    EndSceneRendering(Frame);

    RenderBloom(Frame,
                GameState->PostProcessParams.Bloom,
                Frame->Backend->HDRRenderTargets[0],
                Frame->Backend->HDRRenderTargets[1],
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
                .image = Frame->Backend->SwapchainImage,
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
        vkCmdPipelineBarrier2(Frame->Backend->CmdBuffer, &BeginDependencyInfo); 

        VkRenderingAttachmentInfo ColorAttachment = 
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = Frame->Backend->SwapchainImageView,
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
            .imageView = Frame->Backend->DepthBuffer->View,
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
            .renderArea = { { 0, 0 }, { Frame->RenderWidth, Frame->RenderHeight } },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &ColorAttachment,
            .pDepthAttachment = &DepthAttachment,
            .pStencilAttachment = nullptr,
        };
        vkCmdBeginRendering(Frame->Backend->CmdBuffer, &RenderingInfo);
        
        // Blit
        {
            pipeline_with_layout Pipeline = Renderer->Pipelines[Pipeline_Blit];
            vkCmdBindPipeline(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
            vkCmdPushConstants(Frame->Backend->CmdBuffer, Pipeline.Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 
                               0, sizeof(f32), &GameState->PostProcessParams.Bloom.Strength);

            VkDescriptorSet BlitDescriptorSet = VK_NULL_HANDLE;
            VkDescriptorSetAllocateInfo BlitSetInfo = 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = Frame->Backend->DescriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &Renderer->SetLayouts[SetLayout_Blit],
            };
            VkResult Result = vkAllocateDescriptorSets(VK.Device, &BlitSetInfo, &BlitDescriptorSet);
            Assert(Result == VK_SUCCESS);

            VkDescriptorImageInfo ImageInfos[2] = 
            {
                {
                    .sampler = VK_NULL_HANDLE,
                    .imageView = Frame->Backend->HDRRenderTargets[0]->MipViews[0],
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
                {
                    .sampler = VK_NULL_HANDLE,
                    .imageView = Frame->Backend->HDRRenderTargets[1]->MipViews[0],
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

            vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Layout,
                                    0, 1, &BlitDescriptorSet, 0, nullptr);
            vkCmdDraw(Frame->Backend->CmdBuffer, 3, 1, 0, 0);
        }

        // 3D widget render
        const VkDeviceSize ZeroOffset = 0;
        vkCmdBindVertexBuffers(Frame->Backend->CmdBuffer, 0, 1, &Renderer->GeometryBuffer.VertexMemory.Buffer, &ZeroOffset);
        vkCmdBindIndexBuffer(Frame->Backend->CmdBuffer, Renderer->GeometryBuffer.IndexMemory.Buffer, ZeroOffset, VK_INDEX_TYPE_UINT32);

        pipeline_with_layout GizmoPipeline = Renderer->Pipelines[Pipeline_Gizmo];
        vkCmdBindPipeline(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Pipeline);
        vkCmdBindDescriptorSets(Frame->Backend->CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GizmoPipeline.Layout,
                                0, 1, &Frame->Backend->UniformDescriptorSet, 
                                0, nullptr);

        for (u32 CmdIndex = 0; CmdIndex < Frame->DrawWidget3DCmdCount; CmdIndex++)
        {
            draw_widget3d_cmd* Cmd = Frame->DrawWidget3DCmds + CmdIndex;

            vkCmdPushConstants(Frame->Backend->CmdBuffer, GizmoPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(Cmd->Transform), &Cmd->Transform);
            vkCmdPushConstants(Frame->Backend->CmdBuffer, GizmoPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                               sizeof(Cmd->Transform), sizeof(Cmd->Color), &Cmd->Color);
            vkCmdDrawIndexed(Frame->Backend->CmdBuffer, Cmd->Base.IndexCount, Cmd->Base.InstanceCount, Cmd->Base.IndexOffset, Cmd->Base.VertexOffset, Cmd->Base.InstanceCount);
        }

        pipeline_with_layout UIPipeline = Renderer->Pipelines[Pipeline_UI];
        VkDescriptorSetLayout SetLayout = Frame->Renderer->SetLayouts[SetLayout_SingleCombinedTexturePS];
        VkDescriptorSet FontDescriptorSet = PushImageDescriptor(Frame, SetLayout, 
                                                                Assets->DefaultFontTextureID);
        RenderImmediates(Frame, UIPipeline.Pipeline, UIPipeline.Layout, FontDescriptorSet);

        vkCmdEndRendering(Frame->Backend->CmdBuffer);
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
            .image = Frame->Backend->SwapchainImage,
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
    vkCmdPipelineBarrier2(Frame->Backend->CmdBuffer, &EndDependencyInfo);
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
        InitializeAssets(Assets, GameState->Renderer, &GameState->TransientArena);

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

    if (GameIO->dt >= 0.3f)
    {
        GameIO->dt = 0.0f;
    }

    render_frame* RenderFrame = BeginRenderFrame(GameState->Renderer, GameIO->OutputWidth, GameIO->OutputHeight);
    
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
    
    if (GameIO->Keys[SC_LeftAlt].bIsDown && GameIO->Keys[SC_F4].bIsDown)
    {
        GameIO->bQuitRequested = true;
    }

    if (DoDebugUI(GameState, GameIO, RenderFrame))
    {
        GameIO->Mouse.dP = { 0.0f, 0.0f };
        GameIO->Keys[SC_MouseLeft].TransitionFlags = 0;
    }
    
    UpdateEditor(GameState, GameIO, RenderFrame);
    if (IsValid(GameState->Editor.SelectedEntityID))
    {
        entity* Entity = GameState->World->Entities + GameState->Editor.SelectedEntityID.Value;
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
    
    UpdateAndRenderWorld(GameState->World, GameState->Assets, RenderFrame, GameIO, 
                         &GameState->TransientArena, GameState->Editor.DrawLights);
    GameRender(GameState, GameIO, RenderFrame);
    EndRenderFrame(RenderFrame);

    GameState->FrameID++;
}