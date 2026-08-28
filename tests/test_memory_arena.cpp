#include "relativistic/core/memory_arena.hpp"
#include <cassert>
#include <cstdint>
#include <cmath>

struct alignas(64) Aligned64Struct {
	double values[8];
};

struct alignas(128) Aligned128Struct {
	double values[16];
};

struct ParticleState {
	double position[4];
	double velocity[4];
	double mass;
	uint32_t flags;
};

int main() {
	using namespace Relativistic::Core;

	{
		CacheAlignedArena64<1024 * 1024> arena;

		assert(arena.capacity() == 1024 * 1024);
		assert(arena.allocated_bytes() == 0);
		assert(arena.remaining_bytes() == 1024 * 1024);
		assert(arena.allocation_count() == 0);
		assert(arena.validate_sentinels());

		double* scalar = arena.allocate<double>(1);
		assert(scalar != nullptr);
		assert(reinterpret_cast<uintptr_t>(scalar) % alignof(double) == 0);
		*scalar = 42.0;

		Aligned64Struct* aligned_obj = arena.allocate<Aligned64Struct, 64>(1);
		assert(aligned_obj != nullptr);
		assert(reinterpret_cast<uintptr_t>(aligned_obj) % 64 == 0);
		aligned_obj->values[0] = 3.14159;

		ParticleState* particle = arena.create<ParticleState>(
			ParticleState{
				.position = {0.0, 10.0, 0.0, 0.0},
				.velocity = {1.0, 0.0, 0.0, 0.1},
				.mass = 1.989e30,
				.flags = 0x1
			}
		);
		assert(particle != nullptr);
		assert(particle->mass == 1.989e30);
		assert(particle->position[1] == 10.0);

		assert(arena.validate_sentinels());
		assert(arena.allocated_bytes() > 0);
		assert(arena.allocation_count() == 3);

		arena.reset();
		assert(arena.allocated_bytes() == 0);
		assert(arena.allocation_count() == 0);
		assert(arena.validate_sentinels());
	}

	{
		CacheAlignedArena128<512 * 1024> arena128;
		assert(arena128.validate_sentinels());

		Aligned128Struct* obj128 = arena128.allocate<Aligned128Struct, 128>(4);
		assert(obj128 != nullptr);
		assert(reinterpret_cast<uintptr_t>(obj128) % 128 == 0);

		for (size_t i = 0; i < 4; ++i) {
			for (size_t j = 0; j < 16; ++j) {
				obj128[i].values[j] = static_cast<double>(i * 16 + j);
			}
		}
		assert(arena128.validate_sentinels());
	}

	{
		CacheAlignedArena64<4096> arena;
		const auto marker_start = arena.get_marker();

		double* arr1 = arena.allocate<double>(10);
		assert(arr1 != nullptr);
		const auto marker_mid = arena.get_marker();
		assert(marker_mid > marker_start);

		{
			ScopedArenaMarker scope(arena);
			double* arr2 = arena.allocate<double>(100);
			assert(arr2 != nullptr);
			assert(arena.get_marker() > marker_mid);
		}

		assert(arena.get_marker() == marker_mid);
		arena.rewind_to_marker(marker_start);
		assert(arena.get_marker() == marker_start);
		assert(arena.validate_sentinels());
	}

	{
		LinearMemoryArena<128, 64> small_arena;
		double* p1 = small_arena.allocate<double>(8);
		assert(p1 != nullptr);

		double* p2 = small_arena.allocate<double>(100);
		assert(p2 == nullptr);
		assert(small_arena.validate_sentinels());
	}

	return 0;
}
