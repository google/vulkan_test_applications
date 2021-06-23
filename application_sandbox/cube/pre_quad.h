#ifndef PRE_QUAD_H_
#define PRE_QUAD_H_

#include "support/containers/unique_ptr.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#include "mathfu/matrix.h"

#include <array>

using Mat4x4 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

namespace simple_img {
#include "star.png.h"
}
const auto& src_data = simple_img::texture;

struct alignas(alignof(containers::unique_ptr<vulkan::VkFramebuffer>)) PreQuadData {
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  vulkan::ImagePointer color_staging_img_;
  containers::unique_ptr<vulkan::VkImageView> color_input_view_;
  containers::unique_ptr<vulkan::DescriptorSet> descriptor_set_;
};

struct PreQuadVulkanInfo {
    VkFormat colorFormat;
    VkSampleCountFlagBits num_samples;
    VkViewport viewport;
    VkRect2D scissor;
};

class PreQuad {
public:
  PreQuad(const entry::EntryData* data);

  void InitializeQuadData(vulkan::VulkanApplication* app, containers::Allocator* allocator,
    PreQuadVulkanInfo vulkanInfo, vulkan::VkCommandBuffer* initialization_buffer, size_t num_swapchain_images);
  void InitializeFrameData(vulkan::VulkanApplication* app, PreQuadData* renderData,
    containers::Allocator* allocator, const VkImageView& colorView, size_t frame_index);
  void RecordRenderCmds(vulkan::VulkanApplication* app, PreQuadData* renderData,
    vulkan::VkCommandBuffer& cmdBuffer, size_t frame_index);
  void Update(float time_since_last_render);
  void UpdateRenderData(vulkan::VkQueue* queue, size_t frame_index);

private:
  void InitializeInputImages(vulkan::VulkanApplication* app, PreQuadData* renderData,
    containers::Allocator* allocator);
  void CopyInputImages(vulkan::VulkanApplication* app, PreQuadData* renderData, vulkan::VkCommandBuffer& cmdBuffer, size_t frame_index);

  struct Data {
    std::array<uint8_t, sizeof(src_data.data)> data;
  };

  vulkan::VulkanModel quad_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding descriptor_set_layout_binding_;
  containers::unique_ptr<vulkan::BufferFrameData<Data>> color_data_;
};

#endif
