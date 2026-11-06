#include "relativistic/distributed/gpu_nbody_executor.hpp"
#include "relativistic/render/vulkan_context.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>

#if __has_include("relativistic/shaders/nbody_accel_fp64.spv.hpp")
#include "relativistic/shaders/nbody_accel_fp64.spv.hpp"
#define RELATIVISTIC_HAS_EMBEDDED_NBODY_SPV 1
#endif

namespace Relativistic::Distributed {

namespace {

[[nodiscard]] std::optional<std::filesystem::path> find_nbody_spirv_path() {
	static constexpr const char* directories[] = {
		"shaders",
		"../shaders",
		"./shaders",
		"../../shaders",
		"../share/relativistic-engine/shaders"
	};
	for (const char* directory : directories) {
		const std::filesystem::path candidate = std::filesystem::path(directory) / "nbody_accel_fp64.comp.spv";
		std::error_code ec;
		if (std::filesystem::exists(candidate, ec)) {
			return candidate;
		}
	}
	return std::nullopt;
}

[[nodiscard]] std::optional<std::vector<uint32_t>> load_spirv_file(const std::filesystem::path& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		return std::nullopt;
	}
	const std::streamsize size = file.tellg();
	if (size < 20 || (size % 4) != 0) {
		return std::nullopt;
	}
	std::vector<uint32_t> code(static_cast<size_t>(size) / 4);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(code.data()), size);
	if (!file || code.front() != 0x07230203U) {
		return std::nullopt;
	}
	return code;
}

}

GpuNBodyExecutor::~GpuNBodyExecutor() noexcept {
	shutdown();
}

std::optional<uint32_t> GpuNBodyExecutor::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const noexcept {
	for (uint32_t i = 0; i < memory_properties_.memoryTypeCount; ++i) {
		if ((type_filter & (1U << i)) && (memory_properties_.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return std::nullopt;
}

bool GpuNBodyExecutor::create_buffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& out_buffer,
	VkDeviceMemory& out_memory
) const noexcept {
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device_, &buffer_info, nullptr, &out_buffer) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements mem_reqs{};
	vkGetBufferMemoryRequirements(device_, out_buffer, &mem_reqs);

	const auto type_index = find_memory_type(mem_reqs.memoryTypeBits, properties);
	if (!type_index.has_value()) {
		vkDestroyBuffer(device_, out_buffer, nullptr);
		out_buffer = VK_NULL_HANDLE;
		return false;
	}

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = *type_index;

	if (vkAllocateMemory(device_, &alloc_info, nullptr, &out_memory) != VK_SUCCESS) {
		vkDestroyBuffer(device_, out_buffer, nullptr);
		out_buffer = VK_NULL_HANDLE;
		return false;
	}

	vkBindBufferMemory(device_, out_buffer, out_memory, 0);
	return true;
}

void GpuNBodyExecutor::destroy_buffer(VkBuffer& buffer, VkDeviceMemory& memory, void** mapped) noexcept {
	if (mapped != nullptr && *mapped != nullptr && memory != VK_NULL_HANDLE) {
		vkUnmapMemory(device_, memory);
		*mapped = nullptr;
	}
	if (buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device_, buffer, nullptr);
		buffer = VK_NULL_HANDLE;
	}
	if (memory != VK_NULL_HANDLE) {
		vkFreeMemory(device_, memory, nullptr);
		memory = VK_NULL_HANDLE;
	}
}

bool GpuNBodyExecutor::ensure_input_capacity(size_t body_count) noexcept {
	const VkDeviceSize required_bytes = static_cast<VkDeviceSize>(std::max<size_t>(body_count, 1)) * sizeof(NBodyBodyState);
	if (required_bytes <= input_capacity_bytes_ && input_buffer_ != VK_NULL_HANDLE) {
		return true;
	}

	destroy_buffer(input_buffer_, input_memory_, &input_mapped_);
	input_capacity_bytes_ = 0;

	if (!create_buffer(
		required_bytes,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		input_buffer_,
		input_memory_
	)) {
		return false;
	}

	if (vkMapMemory(device_, input_memory_, 0, required_bytes, 0, &input_mapped_) != VK_SUCCESS) {
		destroy_buffer(input_buffer_, input_memory_);
		return false;
	}

	input_capacity_bytes_ = required_bytes;

	VkDescriptorBufferInfo input_info{};
	input_info.buffer = input_buffer_;
	input_info.offset = 0;
	input_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptor_set_;
	write.dstBinding = 1;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.pBufferInfo = &input_info;

	vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
	return true;
}

bool GpuNBodyExecutor::ensure_output_capacity(size_t row_count) noexcept {
	const VkDeviceSize required_bytes = static_cast<VkDeviceSize>(std::max<size_t>(row_count, 1)) * sizeof(NBodyRowResult);
	if (required_bytes <= output_capacity_bytes_ && output_buffer_ != VK_NULL_HANDLE) {
		return true;
	}

	destroy_buffer(output_buffer_, output_memory_);
	destroy_buffer(staging_buffer_, staging_memory_, &staging_mapped_);
	output_capacity_bytes_ = 0;
	staging_capacity_bytes_ = 0;

	if (!create_buffer(
		required_bytes,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		output_buffer_,
		output_memory_
	)) {
		return false;
	}

	VkBufferCreateInfo staging_info{};
	staging_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	staging_info.size = required_bytes;
	staging_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	staging_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(device_, &staging_info, nullptr, &staging_buffer_) != VK_SUCCESS) {
		destroy_buffer(output_buffer_, output_memory_);
		return false;
	}

	VkMemoryRequirements staging_mem_reqs{};
	vkGetBufferMemoryRequirements(device_, staging_buffer_, &staging_mem_reqs);

	auto staging_type_index = find_memory_type(staging_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	staging_is_coherent_ = false;
	if (!staging_type_index.has_value()) {
		staging_type_index = find_memory_type(staging_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		staging_is_coherent_ = true;
	} else {
		staging_is_coherent_ = (memory_properties_.memoryTypes[*staging_type_index].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
	}

	if (!staging_type_index.has_value()) {
		destroy_buffer(output_buffer_, output_memory_);
		vkDestroyBuffer(device_, staging_buffer_, nullptr);
		staging_buffer_ = VK_NULL_HANDLE;
		return false;
	}

	VkMemoryAllocateInfo staging_alloc{};
	staging_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	staging_alloc.allocationSize = staging_mem_reqs.size;
	staging_alloc.memoryTypeIndex = *staging_type_index;

	if (vkAllocateMemory(device_, &staging_alloc, nullptr, &staging_memory_) != VK_SUCCESS) {
		destroy_buffer(output_buffer_, output_memory_);
		vkDestroyBuffer(device_, staging_buffer_, nullptr);
		staging_buffer_ = VK_NULL_HANDLE;
		return false;
	}

	vkBindBufferMemory(device_, staging_buffer_, staging_memory_, 0);

	if (vkMapMemory(device_, staging_memory_, 0, required_bytes, 0, &staging_mapped_) != VK_SUCCESS) {
		destroy_buffer(output_buffer_, output_memory_);
		destroy_buffer(staging_buffer_, staging_memory_);
		return false;
	}

	output_capacity_bytes_ = required_bytes;
	staging_capacity_bytes_ = required_bytes;

	VkDescriptorBufferInfo output_desc{};
	output_desc.buffer = output_buffer_;
	output_desc.offset = 0;
	output_desc.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptor_set_;
	write.dstBinding = 2;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.pBufferInfo = &output_desc;

	vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
	return true;
}

bool GpuNBodyExecutor::initialize(Render::VulkanContext& context, std::span<const uint32_t> embedded_shader) noexcept {
	if (!context.has_compute_device() || !context.supports_fp64()) {
		return false;
	}

	device_ = context.device();
	physical_device_ = context.physical_device();
	compute_queue_ = context.compute_queue();
	command_pool_ = context.command_pool();
	vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties_);

	std::span<const uint32_t> bytecode = embedded_shader;
#if defined(RELATIVISTIC_HAS_EMBEDDED_NBODY_SPV)
	if (bytecode.empty()) {
		bytecode = std::span<const uint32_t>(::nbody_accel_fp64_spv);
	}
#endif
	std::optional<std::vector<uint32_t>> disk_code;
	if (bytecode.empty()) {
		const auto path = find_nbody_spirv_path();
		if (!path.has_value()) {
			return false;
		}
		disk_code = load_spirv_file(*path);
		if (!disk_code.has_value()) {
			return false;
		}
		bytecode = std::span<const uint32_t>(*disk_code);
	}

	VkShaderModuleCreateInfo shader_info{};
	shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_info.codeSize = bytecode.size() * sizeof(uint32_t);
	shader_info.pCode = bytecode.data();
	if (vkCreateShaderModule(device_, &shader_info, nullptr, &shader_module_) != VK_SUCCESS) {
		return false;
	}

	std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
		return false;
	}

	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;

	if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
		return false;
	}

	VkPipelineShaderStageCreateInfo stage_info{};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = shader_module_;
	stage_info.pName = "main";

	VkComputePipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_info.stage = stage_info;
	pipeline_info.layout = pipeline_layout_;

	if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &compute_pipeline_) != VK_SUCCESS) {
		return false;
	}

	std::array<VkDescriptorPoolSize, 2> pool_sizes{};
	pool_sizes[0] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
	pool_sizes[1] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2};

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.maxSets = 1;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();

	if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
		return false;
	}

	VkDescriptorSetAllocateInfo set_alloc{};
	set_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_alloc.descriptorPool = descriptor_pool_;
	set_alloc.descriptorSetCount = 1;
	set_alloc.pSetLayouts = &descriptor_set_layout_;

	if (vkAllocateDescriptorSets(device_, &set_alloc, &descriptor_set_) != VK_SUCCESS) {
		return false;
	}

	if (!create_buffer(
		sizeof(GpuNBodyUniformParameters),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniform_buffer_,
		uniform_memory_
	)) {
		return false;
	}

	if (vkMapMemory(device_, uniform_memory_, 0, sizeof(GpuNBodyUniformParameters), 0, &uniform_mapped_) != VK_SUCCESS) {
		return false;
	}

	VkDescriptorBufferInfo uniform_desc{};
	uniform_desc.buffer = uniform_buffer_;
	uniform_desc.offset = 0;
	uniform_desc.range = sizeof(GpuNBodyUniformParameters);

	VkWriteDescriptorSet uniform_write{};
	uniform_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniform_write.dstSet = descriptor_set_;
	uniform_write.dstBinding = 0;
	uniform_write.descriptorCount = 1;
	uniform_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniform_write.pBufferInfo = &uniform_desc;

	vkUpdateDescriptorSets(device_, 1, &uniform_write, 0, nullptr);

	if (!ensure_input_capacity(64) || !ensure_output_capacity(64)) {
		return false;
	}

	VkCommandBufferAllocateInfo cmd_alloc{};
	cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_alloc.commandPool = command_pool_;
	cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_alloc.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device_, &cmd_alloc, &command_buffer_) != VK_SUCCESS) {
		return false;
	}

	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(device_, &fence_info, nullptr, &fence_) != VK_SUCCESS) {
		return false;
	}

	ready_ = true;
	device_lost_ = false;
	return true;
}

void GpuNBodyExecutor::shutdown() noexcept {
	if (device_ == VK_NULL_HANDLE) {
		ready_ = false;
		return;
	}
	if (!device_lost_) {
		vkDeviceWaitIdle(device_);
	}

	if (fence_ != VK_NULL_HANDLE) { vkDestroyFence(device_, fence_, nullptr); fence_ = VK_NULL_HANDLE; }
	if (command_buffer_ != VK_NULL_HANDLE && command_pool_ != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer_);
		command_buffer_ = VK_NULL_HANDLE;
	}

	destroy_buffer(uniform_buffer_, uniform_memory_, &uniform_mapped_);
	destroy_buffer(input_buffer_, input_memory_, &input_mapped_);
	destroy_buffer(output_buffer_, output_memory_);
	destroy_buffer(staging_buffer_, staging_memory_, &staging_mapped_);

	input_capacity_bytes_ = 0;
	output_capacity_bytes_ = 0;
	staging_capacity_bytes_ = 0;

	if (descriptor_pool_ != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr); descriptor_pool_ = VK_NULL_HANDLE; }
	if (compute_pipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_, compute_pipeline_, nullptr); compute_pipeline_ = VK_NULL_HANDLE; }
	if (pipeline_layout_ != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr); pipeline_layout_ = VK_NULL_HANDLE; }
	if (descriptor_set_layout_ != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr); descriptor_set_layout_ = VK_NULL_HANDLE; }
	if (shader_module_ != VK_NULL_HANDLE) { vkDestroyShaderModule(device_, shader_module_, nullptr); shader_module_ = VK_NULL_HANDLE; }

	device_ = VK_NULL_HANDLE;
	physical_device_ = VK_NULL_HANDLE;
	compute_queue_ = VK_NULL_HANDLE;
	command_pool_ = VK_NULL_HANDLE;
	ready_ = false;
}

bool GpuNBodyExecutor::evaluate(
	std::span<const NBodyBodyState> bodies,
	uint32_t first_row,
	uint32_t row_count,
	uint32_t phase,
	double gravitational_constant,
	double speed_of_light,
	std::span<NBodyRowResult> output
) noexcept {
	if (!is_ready() || bodies.empty() || row_count == 0U || output.size() < row_count) {
		return false;
	}
	if (static_cast<size_t>(first_row) + row_count > bodies.size()) {
		return false;
	}

	if (!ensure_input_capacity(bodies.size()) || !ensure_output_capacity(row_count)) {
		return false;
	}

	GpuNBodyUniformParameters parameters{};
	parameters.total_bodies = static_cast<uint32_t>(bodies.size());
	parameters.first_row = first_row;
	parameters.row_count = row_count;
	parameters.phase = phase;
	parameters.gravitational_constant = gravitational_constant;
	parameters.speed_of_light = speed_of_light;

	std::memcpy(uniform_mapped_, &parameters, sizeof(parameters));
	std::memcpy(input_mapped_, bodies.data(), bodies.size() * sizeof(NBodyBodyState));

	if (vkResetCommandBuffer(command_buffer_, 0) != VK_SUCCESS) {
		return false;
	}

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(command_buffer_, &begin_info) != VK_SUCCESS) {
		return false;
	}

	vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_);
	vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1, &descriptor_set_, 0, nullptr);

	const uint32_t workgroups = (row_count + 63U) / 64U;
	vkCmdDispatch(command_buffer_, workgroups, 1U, 1U);

	VkBufferMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = output_buffer_;
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(command_buffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = static_cast<VkDeviceSize>(row_count) * sizeof(NBodyRowResult);
	vkCmdCopyBuffer(command_buffer_, output_buffer_, staging_buffer_, 1, &copy_region);

	if (vkEndCommandBuffer(command_buffer_) != VK_SUCCESS) {
		return false;
	}

	if (vkResetFences(device_, 1, &fence_) != VK_SUCCESS) {
		return false;
	}

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer_;

	if (vkQueueSubmit(compute_queue_, 1, &submit_info, fence_) != VK_SUCCESS) {
		device_lost_ = true;
		ready_ = false;
		return false;
	}

	constexpr uint64_t kTimeoutNs = 5000000000ULL;
	if (vkWaitForFences(device_, 1, &fence_, VK_TRUE, kTimeoutNs) != VK_SUCCESS) {
		device_lost_ = true;
		ready_ = false;
		return false;
	}

	if (!staging_is_coherent_) {
		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = staging_memory_;
		range.offset = 0;
		range.size = VK_WHOLE_SIZE;
		vkInvalidateMappedMemoryRanges(device_, 1, &range);
	}

	std::memcpy(output.data(), staging_mapped_, static_cast<size_t>(copy_region.size));
	return true;
}

}
