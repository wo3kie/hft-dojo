/*
 * Website:
 *      HFTDojo https://github.com/wo3kie/hft-dojo
 *
 * Author:
 *      Lukasz Czerwinski (https://www.lukaszczerwinski.pl/)
 */

#include "assert.hpp"
#include "ring_buffer_mutex.hpp"
#include "test_utils.hpp"

namespace {

struct MoveOnly {
	int value = 0;

	MoveOnly() = default;
	explicit MoveOnly(int value): value(value) {
	}

	MoveOnly(const MoveOnly&) = delete;
	MoveOnly& operator=(const MoveOnly&) = delete;

	MoveOnly(MoveOnly&& other) noexcept: value(other.value) {
		other.value = -1;
	}

	MoveOnly& operator=(MoveOnly&& other) noexcept {
		if(this != &other) {
			value = other.value;
			other.value = -1;
		}

		return *this;
	}
};

template<std::size_t Capacity>
void test_empty_full_and_fifo_without_blocking() {
	RingBufferMutex<int, Capacity> buffer;

	Assert(buffer.capacity() == Capacity);
	Assert(buffer.empty() == true);
	Assert(buffer.full() == false);

	for(std::size_t i = 0; i < Capacity; ++i) {
		Assert(buffer.push(static_cast<int>(i + 10)) == true);
	}

	Assert(buffer.empty() == false);
	Assert(buffer.full() == true);

	int out = -1;

	for(std::size_t i = 0; i < Capacity; ++i) {
		Assert(buffer.pop(out) == true);
		Assert(out == static_cast<int>(i + 10));
	}

	Assert(buffer.empty() == true);
	Assert(buffer.full() == false);
}

template<std::size_t Capacity>
void test_wrap_around_without_blocking() {
	RingBufferMutex<int, Capacity> buffer;
	int out = -1;

	for(std::size_t i = 0; i < Capacity; ++i) {
		Assert(buffer.push(static_cast<int>(i)) == true);
	}

	for(std::size_t i = 0; i < Capacity / 2; ++i) {
		Assert(buffer.pop(out) == true);
		Assert(out == static_cast<int>(i));
	}

	for(std::size_t i = 0; i < Capacity / 2; ++i) {
		Assert(buffer.push(static_cast<int>(100 + i)) == true);
	}

	for(std::size_t i = Capacity / 2; i < Capacity; ++i) {
		Assert(buffer.pop(out) == true);
		Assert(out == static_cast<int>(i));
	}

	for(std::size_t i = 0; i < Capacity / 2; ++i) {
		Assert(buffer.pop(out) == true);
		Assert(out == static_cast<int>(100 + i));
	}

	Assert(buffer.empty() == true);
}

void test_ext_pop_on_non_empty_buffer() {
	RingBufferMutex<int, 4> buffer;

	Assert(buffer.push(42) == true);
	Assert(buffer._ext_pop() == 42);
	Assert(buffer.empty() == true);
}

void test_move_only_values() {
	RingBufferMutex<MoveOnly, 4> buffer;

	MoveOnly pushed(42);
	Assert(buffer.push(std::move(pushed)) == true);
	Assert(pushed.value == -1);

	MoveOnly out;
	Assert(buffer.pop(out) == true);
	Assert(out.value == 42);
	Assert(buffer.empty() == true);
}

} // namespace

int main() {
	test_empty_full_and_fifo_without_blocking<3>();
	test_empty_full_and_fifo_without_blocking<4>();
	test_wrap_around_without_blocking<4>();
	test_wrap_around_without_blocking<5>();
	test_ext_pop_on_non_empty_buffer();
	test_move_only_values();

}
