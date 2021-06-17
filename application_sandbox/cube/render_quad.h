#include "support/containers/unique_ptr.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#include "mathfu/matrix.h"

#include <array>

using Mat4x4 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

struct alignas(alignof(containers::unique_ptr<vulkan::VkFramebuffer>)) RenderQuadData {
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> descriptor_set_;
};

struct QuadVulkanInfo {
    VkFormat colorFormat;
    VkSampleCountFlagBits num_samples;
    VkViewport viewport;
    VkRect2D scissor;
};

class RenderQuad {
public:
	RenderQuad(const entry::EntryData* data);

 	void InitializeQuadData(vulkan::VulkanApplication* app, containers::Allocator* allocator, 
    QuadVulkanInfo vulkanInfo, vulkan::VkCommandBuffer* initialization_buffer, size_t num_swapchain_images);
 	void InitializeFrameData(vulkan::VulkanApplication* app, RenderQuadData* renderData, 
    containers::Allocator* allocator, const VkImageView& colorView, const VkImageView& inputView, size_t frame_index);
  void RecordRenderCmds(vulkan::VulkanApplication* app, RenderQuadData* renderData, 
    vulkan::VkCommandBuffer& cmdBuffer, size_t frame_index);
 	void Update(float time_since_last_render);
 	void UpdateRenderData(vulkan::VkQueue* queue, size_t frame_index);

private:
  vulkan::VulkanModel quad_;
  containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> pipeline_;
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
  VkDescriptorSetLayoutBinding descriptor_set_layout_binding_;
};
