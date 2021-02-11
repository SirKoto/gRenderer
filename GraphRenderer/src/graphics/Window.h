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

		void initialize(int width, int heigth, const std::string& windowTitle);

		int getWidth() const { return mWidth; }
		int getHeigth() const {	return mHeight; }

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
			InputCOUNT
		};

		bool isJustPressed(Input in) const;
		bool isPressed(Input in) const;
		bool isDown(Input in) const;

		double getMouseWheelOffset() const { return mMouseWheelBuff; }

		void getMousePosition(std::array<double, 2>* pos) const;

	private:

		int mWidth, mHeight;
		void* mWindow;
		vk::SurfaceKHR mSurface;

		struct InputState {
			uint8_t justPressed : 1;
			uint8_t pressed : 1;

			InputState() : justPressed(0), pressed(0) {}
		};

		std::array<InputState, static_cast<size_t>(Input::InputCOUNT)> mInputState;
		double mMouseWheel = 0, mMouseWheelBuff;

		static void s_mouseWheelCallback(void*, double, double);

	};

}; // namespace vkg
}; // namespace gr
