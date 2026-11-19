# Convenience wrapper around the CMake workflow.
# PROJECT is parsed from the project() line of the root CMakeLists.txt, so
# renaming the project requires no Makefile edits.
# The sed program uses \x28 (hex escape for '(') instead of a literal '(':
# make counts raw parens inside $(shell ...) and an unmatched one aborts the
# parse — only the \( \) capture group below stays balanced for make.
PROJECT   := $(shell sed -n 's/^project\x28\([^ ]*\).*/\1/p' CMakeLists.txt)
BUILD_DIR := build
BINARY    := $(BUILD_DIR)/src/$(PROJECT)
TEST_BIN  := $(BUILD_DIR)/tests/$(PROJECT)_test
CMAKE_ARGS := -DCMAKE_BUILD_TYPE=Debug
JOBS      := $(shell nproc)

.PHONY: all configure build run test clean rebuild

all: build

# ── Configure ──────────────────────────────────────────────────────────────
configure:
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS)
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

# ── Build ──────────────────────────────────────────────────────────────────
build: configure
	cmake --build $(BUILD_DIR) -j $(JOBS)

# ── Run ────────────────────────────────────────────────────────────────────
run: build
	$(BINARY)

# ── Test ───────────────────────────────────────────────────────────────────
test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

# ── Single-test convenience ────────────────────────────────────────────────
.PHONY: test-one
test-one: build
	$(TEST_BIN) "$(name)"

# ── Clean ──────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR) compile_commands.json

# ── Rebuild ────────────────────────────────────────────────────────────────
rebuild: clean build
