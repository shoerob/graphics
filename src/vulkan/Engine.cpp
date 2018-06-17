#include "Graphics/Engine.h"
#include "ErrorTranslation.h"
#include "vulkan/vulkan.h"
#include <exception>
#include <iostream>
#include <memory>
#include <vector>

using namespace CR::Graphics;
using namespace std;
using namespace std::string_literals;

namespace {
	constexpr uint MajorVersion = 0;    // 64K max
	constexpr uint MinorVersion = 1;    // 256 max
	constexpr uint PatchVersion = 1;    // 256 max
	constexpr uint Version      = (MajorVersion << 16) || (MajorVersion << 8) || (PatchVersion);

	class Engine {
	  public:
		Engine(const EngineSettings& a_settings);
		~Engine();
		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

	  private:
		VkInstance m_Instance{nullptr};
		VkDevice m_Device;
	};

	unique_ptr<Engine> g_Engine;
}    // namespace

Engine::Engine(const EngineSettings& a_settings) {
	vector<string> enabledLayers;
	if(a_settings.EnableDebug) {
		uint numLayers;
		vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
		vector<VkLayerProperties> layers;
		layers.resize(numLayers);
		vkEnumerateInstanceLayerProperties(&numLayers, data(layers));
		for(uint i = 0; i < numLayers; ++i) {
			if("VK_LAYER_LUNARG_standard_validation"s == layers[i].layerName) {
				enabledLayers.push_back(layers[i].layerName);
			}
		}
	}

	VkApplicationInfo appInfo;
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext              = nullptr;
	appInfo.pApplicationName   = a_settings.ApplicationName.c_str();
	appInfo.applicationVersion = a_settings.ApplicationVersion;
	appInfo.pEngineName        = "Conjure";
	appInfo.engineVersion      = Version;
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	vector<const char*> enabledLayersPtrs;
	for(auto& layer : enabledLayers) { enabledLayersPtrs.push_back(layer.c_str()); }

	VkInstanceCreateInfo createInfo;
	createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext                   = nullptr;
	createInfo.flags                   = 0;
	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledLayerCount       = (uint32_t)size(enabledLayersPtrs);
	createInfo.ppEnabledLayerNames     = data(enabledLayersPtrs);
	createInfo.enabledExtensionCount   = 0;
	createInfo.ppEnabledExtensionNames = nullptr;

	TranslateError(vkCreateInstance(&createInfo, nullptr, &m_Instance));

	vector<VkPhysicalDevice> physicalDevices;
	uint32_t deviceCount;
	TranslateError(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr));
	physicalDevices.resize(deviceCount);
	TranslateError(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, data(physicalDevices)));

	VkPhysicalDevice selectedDevice;
	bool foundDevice       = false;
	int graphicsQueueIndex = -1;
	int transferQueueIndex = -1;
	for(auto& device : physicalDevices) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(device, &props);
		cout << "Device Name: " << props.deviceName << endl;
		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(device, &memProps);
		for(uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
			if(memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
				cout << "Device Local Memory Amount: "
				     << memProps.memoryHeaps[memProps.memoryTypes[i].heapIndex].size / (1024 * 1024) << "MB" << endl;
			}
		}

		uint32_t queueFamilyCount;
		vector<VkQueueFamilyProperties> queueProps;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		queueProps.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, data(queueProps));
		bool supportsGraphics = false;
		bool supportsTransfer = false;
		for(int i = 0; i < queueProps.size(); ++i) {
			// This one should only be false for tesla compute cards and similiar
			if((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && queueProps[i].queueCount >= 1) {
				supportsGraphics   = true;
				graphicsQueueIndex = i;
			}
			if((queueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && queueProps[i].queueCount >= 1) {
				supportsTransfer   = true;
				transferQueueIndex = i;
			}
		}

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);

		// TODO: We dont have a good heuristic for selecting a device, for now just take first one that supports
		// graphics and hope for the best.  My machine has only one, so cant test a better implementation.
		if(supportsGraphics && supportsTransfer && features.textureCompressionBC && features.fullDrawIndexUint32) {
			foundDevice    = true;
			selectedDevice = device;
			break;
		}
	}
	if(!foundDevice) { throw runtime_error("Could not find a valid vulkan device"); }

	VkPhysicalDeviceFeatures requiredFeatures;
	memset(&requiredFeatures, 0, sizeof(requiredFeatures));
	requiredFeatures.textureCompressionBC = true;
	requiredFeatures.fullDrawIndexUint32  = true;

	float graphicsPriority = 1.0f;
	float transferPriority = 0.0f;
	VkDeviceQueueCreateInfo queueInfos[2];
	queueInfos[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfos[0].pNext            = nullptr;
	queueInfos[0].flags            = 0;
	queueInfos[0].queueFamilyIndex = graphicsQueueIndex;
	queueInfos[0].queueCount       = 1;
	queueInfos[0].pQueuePriorities = &graphicsPriority;
	queueInfos[1]                  = queueInfos[0];
	queueInfos[1].queueFamilyIndex = transferQueueIndex;
	queueInfos[1].pQueuePriorities = &transferPriority;

	VkDeviceCreateInfo createLogDevInfo;
	createLogDevInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createLogDevInfo.pNext                   = nullptr;
	createLogDevInfo.flags                   = 0;
	createLogDevInfo.queueCreateInfoCount    = (int)size(queueInfos);
	createLogDevInfo.pQueueCreateInfos       = queueInfos;
	createLogDevInfo.pEnabledFeatures        = &requiredFeatures;
	createLogDevInfo.enabledLayerCount       = 0;
	createLogDevInfo.ppEnabledLayerNames     = nullptr;
	createLogDevInfo.enabledExtensionCount   = 0;
	createLogDevInfo.ppEnabledExtensionNames = nullptr;

	TranslateError(vkCreateDevice(selectedDevice, &createLogDevInfo, nullptr, &m_Device));
}

Engine::~Engine() {
	vkDestroyInstance(m_Instance, nullptr);
}

void CR::Graphics::CreateEngine(const EngineSettings& a_settings) {
	g_Engine = make_unique<Engine>(a_settings);
}

void CR::Graphics::ShutdownEngine() {
	g_Engine.reset();
}