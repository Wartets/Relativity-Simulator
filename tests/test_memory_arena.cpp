#include "support/test_harness.hpp"
#include "relativistic/core/memory_arena.hpp"
#include <cstdint>

using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

namespace {

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

void check_allocation(TestSuite& suite) {
	suite.section("Allocation and alignment");
	CacheAlignedArena64<1024 * 1024> arena;
	suite.check(arena.capacity() == 1024 * 1024 && arena.allocated_bytes() == 0 && arena.remaining_bytes() == 1024 * 1024, "a new arena is empty with its full capacity");
	suite.check(arena.allocation_count() == 0 && arena.validate_sentinels(), "a new arena has no allocations and intact sentinels");

	double* scalar = arena.allocate<double>(1);
	suite.check(scalar != nullptr && reinterpret_cast<uintptr_t>(scalar) % alignof(double) == 0, "a scalar allocation is naturally aligned");
	Aligned64Struct* aligned = arena.allocate<Aligned64Struct, 64>(1);
	suite.check(aligned != nullptr && reinterpret_cast<uintptr_t>(aligned) % 64 == 0, "a 64-byte aligned allocation honours its alignment");
	ParticleState* particle = arena.create<ParticleState>(ParticleState{{0.0, 10.0, 0.0, 0.0}, {1.0, 0.0, 0.0, 0.1}, 1.989e30, 0x1});
	suite.check(particle != nullptr && particle->mass == 1.989e30 && particle->position[1] == 10.0, "create constructs the object with its initial state");
	suite.check(arena.allocation_count() == 3 && arena.allocated_bytes() > 0 && arena.validate_sentinels(), "the allocation count and sentinels remain consistent");
	arena.reset();
	suite.check(arena.allocated_bytes() == 0 && arena.allocation_count() == 0 && arena.validate_sentinels(), "reset releases every allocation");

	CacheAlignedArena128<512 * 1024> wide;
	Aligned128Struct* aligned_128 = wide.allocate<Aligned128Struct, 128>(4);
	suite.check(aligned_128 != nullptr && reinterpret_cast<uintptr_t>(aligned_128) % 128 == 0, "a 128-byte aligned array honours its alignment");
	if (aligned_128 != nullptr) {
		for (size_t i = 0; i < 4; ++i) {
			for (size_t j = 0; j < 16; ++j) {
				aligned_128[i].values[j] = static_cast<double>(i * 16 + j);
			}
		}
	}
	suite.check(wide.validate_sentinels(), "writing the whole allocation does not corrupt the sentinels");
}

void check_markers(TestSuite& suite) {
	suite.section("Markers and scopes");
	CacheAlignedArena64<4096> arena;
	const auto start = arena.get_marker();
	double* first = arena.allocate<double>(10);
	const auto middle = arena.get_marker();
	suite.check(first != nullptr && middle > start, "an allocation advances the marker");
	{
		ScopedArenaMarker scope(arena);
		double* nested = arena.allocate<double>(100);
		suite.check(nested != nullptr && arena.get_marker() > middle, "a scoped allocation advances the marker");
	}
	suite.check(arena.get_marker() == middle, "leaving the scope restores the marker");
	arena.rewind_to_marker(start);
	suite.check(arena.get_marker() == start && arena.validate_sentinels(), "rewinding returns to the initial marker");
}

void check_exhaustion(TestSuite& suite) {
	suite.section("Exhaustion");
	LinearMemoryArena<128, 64> small;
	suite.check(small.allocate<double>(8) != nullptr, "an allocation within capacity succeeds");
	suite.check(small.allocate<double>(100) == nullptr, "an allocation beyond capacity returns null");
	suite.check(small.validate_sentinels(), "a failed allocation leaves the sentinels intact");
}

}

int main() {
	TestSuite suite(
		"memory_arena",
		"Verifies the linear memory arenas used by the zero-allocation hot path: capacity accounting, natural and cache-line alignment, object creation, reset, markers, scoped rewinding, sentinel integrity and exhaustion handling."
	);
	check_allocation(suite);
	check_markers(suite);
	check_exhaustion(suite);
	return suite.finish();
}
