#pragma once

#include <vector>

#include "../IObject.h"
#include "../Mesh.h"

namespace gr
{

class VisibilityGrid : public IObject
{
public:

	VisibilityGrid();

	void start(FrameContext* fc) override final;
	void scheduleDestroy(FrameContext* fc) override final;
	void renderImGui(FrameContext* fc, Gui* gui) override final;


private:
	

	class Cell {
	public:
		Cell();

		enum class Dir : uint32_t {
			eUp = 0,
			eRight = 1,
			eDown = 2, 
			eLeft = 3
		};

		uint8_t& get(Dir d) { return occupied[(uint32_t)d]; }
		const uint8_t& get(Dir d) const { return occupied[(uint32_t)d]; }

		uint8_t& up() { return get(Dir::eUp); }
		const uint8_t& up() const { return get(Dir::eUp); }
		uint8_t& right() { return get(Dir::eRight); }
		const uint8_t& right() const { return get(Dir::eRight); }
		uint8_t& down() { return get(Dir::eDown); }
		const uint8_t& down() const { return get(Dir::eDown); }
		uint8_t& left() { return get(Dir::eLeft); }
		const uint8_t& left() const { return get(Dir::eLeft); }


	private:
		std::array<uint8_t, 4> occupied;
	};

	std::vector<std::vector<Cell>> mWallsGrid;
	uint32_t mResolutionX, mResolutionY;

	std::unique_ptr<Mesh> mMesh;

};

} // namespace gr
