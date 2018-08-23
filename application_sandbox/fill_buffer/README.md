# Fill Buffer

After 300 frames changes the color output by filling a buffer with new values.

Standard usage should be of the form
```c++
struct PerFrameSampleData {
};

class MySample : public sample_application::Sample<PerFrameSampleData> {
    void InitializeApplicationData(
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t num_swapchain_images) override {}
    void InitializeFrameData(
      PerFrameSampleData* frame_data,
      vulkan::VkCommandBuffer* initialization_buffer,
      size_t frame_index) override {}
    void Update(float time_since_last_render) {}
    void Render(vulkan::VkQueue* queue, size_t frame_index,
                      FillFrameData* frame_data) {}
}
int main_entry(const entry::EntryData* data) {
  MySample sample(data);
  sample.Initialize();

  while (!sample.should_exit() && !data->ShouldExit()) { !data->ShouldExit()) {
    sample.ProcessFrame();
  }
  sample.WaitIdle();
  return 0;
}
```

Where `PerFrameSampleData` is the set of data that is buffered per frame.
This is typically command-buffers, descriptor sets, framebuffers.

Your application will receieve a callback to `InitializeApplicationData` during
initialization. Any data and initalization that needs to be done can be done
there. The given `initialization_buffer` will already be in the recording
state and can be used to fill in initial data. This is where models/textures
should be intialized.

Your application will receive a callback to `InitializeFrameData` for each
swapchain image that the sample framework has decided should be created.
At that point the data can be filled in. The given `initialization_buffer`
will already be in the recording state and can be used to fill initial data.
And frame-specific data should be initialized here.

After that `Update()` and `Render()` will both be called. Currently they are
both called once per frame. Update should update any data, and Render should
make sure that data is presented on the screen.

Typically `vulkan::BufferFrameData` objects values are set in `Update` and
then commited to the GPU in `Render()`.