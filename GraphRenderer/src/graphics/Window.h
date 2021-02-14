#pragma once
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>


namespace gr
{
namespace vkg
{

	class Window
	{
	public:

		Window& operator=(const Window& w) = delete;
		Window& operator=(Window&& w) = delete;

		struct WindowConfig {
			const char* windowTitle = "";
			bool resizableWindow = true;
		};

		void initialize(int width, int heigth, const WindowConfig& config);

		int32_t getWidth() const { return mWidth; }
		int32_t getHeigth() const {	return mHeight; }
		int32_t getFrameBufferWidth() const { return mPixelWidth; }
		int32_t getFrameBufferHeigth() const { return mPixelHeight; }

		void createVkSurface(
			const vk::Instance& instance
		);

		bool windowShouldClose() const;

		const vk::SurfaceKHR& getSurface() const { return mSurface; }

		bool isWindowMinimized() const;

		void destroy(const vk::Instance& instance);

		void* getWindow() const { return mWindow; }

		void update();

		static void pollEvents();

		static void waitEvents();

		static std::vector<const char*> getWindowVkExtensions();

		enum class Input {
			MouseLeft,
			MouseRight,
			KeyA,
			KeyB,
			KeyC,
			KeyD,
			KeyE,
			KeyF,
			KeyG,
			KeyH,
			KeyI,
			KeyJ,
			KeyK,
			KeyL,
			KeyM,
			KeyN,
			KeyO,
			KeyP,
			KeyQ,
			KeyR,
			KeyS,
			KeyT,
			KeyU,
			KeyV,
			KeyW,
			KeyX,
			KeyY,
			KeyZ,
			ArrowLeft,
			ArrowDown,
			ArrowRight,
			ArrowUp,
			KeyBackspace,
			KeyDelete,
			KeyTab,
			KeyHome,
			KeyEnd,
			KeySpace,
			KeyEnter,
			KeyEnterKeyPad,
			KeyEscape,
			KeyCtrl,
			KeyShift,
			KeyAlt,
			InputCOUNT
		};

		bool isJustPressed(Input in) const;
		bool isPressed(Input in) const;
		bool isDown(Input in) const;

		double getMouseWheelOffset() const { return mMouseWheelBuff; }

		void getMousePosition(std::array<double, 2>* pos) const;

		const std::vector<uint32_t>& getInputCharsUTF() const { return mCharInputBuff; }

	private:

		int32_t mWidth, mHeight;
		int32_t mPixelWidth, mPixelHeight;
		void* mWindow;
		vk::SurfaceKHR mSurface;

		struct InputState {
			uint8_t justPressed : 1;
			uint8_t pressed : 1;

			InputState() : justPressed(0), pressed(0) {}
		};

		std::array<InputState, static_cast<size_t>(Input::InputCOUNT)> mInputState;
		double mMouseWheel = 0, mMouseWheelBuff;
		std::vector<uint32_t> mCharInput, mCharInputBuff;

		static void s_mouseWheelCallback(void*, double, double);
		static void s_charCallback(void*, unsigned int);
		static void s_windowSizeCallback(void*, int, int);
		static void s_windowFramebufferSizeCallback(void*, int, int);

	};

}; // namespace vkg
}; // namespace gr
