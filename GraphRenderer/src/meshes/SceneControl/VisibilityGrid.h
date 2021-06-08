#pragma once

#include <vector>
#include <unordered_map>

#include "../IObject.h"
#include "../Mesh.h"
#include "../GameObject.h"

namespace gr
{

class VisibilityGrid : public IObject
{
public:

	VisibilityGrid();

	void start(FrameContext* fc) override final;
	void scheduleDestroy(FrameContext* fc) override final;
	void renderImGui(FrameContext* fc, Gui* gui) override final;
	void graphicsUpdate(FrameContext* fc, const SceneRenderContext& src);
	void logicUpdate(FrameContext* fc);
	void computeVisibility(FrameContext* fc, const std::set<ResId>& gameObjects);


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

	std::vector<std::vector<Cell>> mWallsCells;
	uint32_t mResolutionX, mResolutionY;

	ResId mMesh;

	struct WallKey {
		uint32_t x;
		uint32_t y;
		bool vertical;

		bool operator==(const WallKey& o) const {
			return this->x == o.x && this->y == o.y && this->vertical == o.vertical;
		}
	};
	struct WallKeyHasher
	{
		std::size_t operator()(const WallKey& k) const
		{
			using std::hash;

			return ((hash<uint32_t>()(k.x)
				^ (hash<uint32_t>()(k.y) << 1)) >> 1)
				^ (hash<bool>()(k.vertical) << 1);
		}
	};
	std::unordered_map<WallKey, std::unique_ptr<GameObject>, WallKeyHasher> mWallsGameObjects;

	std::vector<std::vector<std::set<ResId>>> mVisibilityGrid;

	void updateWallCellGameObject(FrameContext* fc, uint32_t x, uint32_t y);

};

} // namespace gr
