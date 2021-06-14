#include "support/containers/unique_ptr.h"
#include "vulkan_helpers/buffer_frame_data.h"
#include "vulkan_helpers/vulkan_application.h"
#include "vulkan_helpers/vulkan_model.h"

#include "mathfu/matrix.h"

using Mat4x4 = mathfu::Matrix<float, 4, 4>;
using Vector4 = mathfu::Vector<float, 4>;

struct CubeRenderData {
  containers::unique_ptr<vulkan::VkFramebuffer> framebuffer_;
  containers::unique_ptr<vulkan::DescriptorSet> cube_descriptor_set_;
};

struct CubeVulkanInfo {
    VkFormat format;
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
 		VkImageView colorView,
    size_t frame_index);

  void RecordRenderCmds(vulkan::VulkanApplication* app,
    CubeRenderData* renderData,
    vulkan::VkCommandBuffer& cmdBuffer);

 	void Update(float time_since_last_render);
 	void Render(vulkan::VkQueue* queue, size_t frame_index);

private:
	struct CameraData {
    	Mat4x4 projection_matrix;
  	};

  	struct ModelData {
    	Mat4x4 transform;
  	};

	vulkan::VulkanModel cube_;
	VkDescriptorSetLayoutBinding cube_descriptor_set_layouts_[2];
	containers::unique_ptr<vulkan::PipelineLayout> pipeline_layout_;
	containers::unique_ptr<vulkan::VulkanGraphicsPipeline> cube_pipeline_;
	
  containers::unique_ptr<vulkan::BufferFrameData<CameraData>> camera_data_;
	containers::unique_ptr<vulkan::BufferFrameData<ModelData>> model_data_;
  
  containers::unique_ptr<vulkan::VkRenderPass> render_pass_;
};
