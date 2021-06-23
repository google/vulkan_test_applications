#ifndef CUBE_RENDER_H_
#define CUBE_RENDER_H_

#include "support/containers/unique_ptr.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#include "mathfu/matrix.h"

using Mat4x4 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

struct CubeRenderData {
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> first_pass_descriptor_set_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;
};

struct CubeVulkanInfo {
    VkFormat colorFormat;
    VkFormat depthFormat;
    VkSampleCountFlagBits num_samples;
    VkViewport viewport;
    VkRect2D scissor;
};

class CubeRender {
public:
	CubeRender(const entry::EntryData* data);

 	void InitializeCubeData(vulkan::VulkanApplication* app,
 		containers::Allocator* allocator,
 		CubeVulkanInfo vulkanInfo,
 		vulkan::VkCommandBuffer* initialization_buffer,
      	size_t num_swapchain_images);

 	void InitializeFrameData(vulkan::VulkanApplication* app,
    CubeRenderData* renderData,
    containers::Allocator* allocator,
    const VkImageView& inputView,
    const VkImageView& colorView,
    const VkImageView& depthView,
    size_t frame_index);

  void RecordRenderCmds(vulkan::VulkanApplication* app,
    CubeRenderData* renderData,
    vulkan::VkCommandBuffer& cmdBuffer);

 	void Update(float time_since_last_render);
 	void UpdateRenderData(vulkan::VkQueue* queue, size_t frame_index);

private:
  void ClearDepthStencilAttachments(vulkan::VulkanApplication* app, vulkan::VkCommandBuffer& cmdBuffer);
  void ClearColorAttachments(vulkan::VulkanApplication* app, vulkan::VkCommandBuffer& cmdBuffer);

	struct CameraData {
    	Mat4x4 projection_matrix;
  	};

  	struct ModelData {
    	Mat4x4 transform;
  	};

	vulkan::VulkanModel cube_;
  vulkan::VulkanModel quad_;
  VkDescriptorSetLayoutBinding first_pass_descriptor_set_layout;
	VkDescriptorSetLayoutBinding descriptor_set_layouts_[2];

  containers::unique_ptr<vulkan::PipelineLayout> first_pass_pipeline_layout_;
  containers::unique_ptr<vulkan::PipelineLayout> cube_pipeline_layout_;
	containers::unique_ptr<vulkan::VulkanGraphicsPipeline> first_pass_pipeline_;
  containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
	
  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
	containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
  
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
};

#endif
