# ============================================================================
# 打字练习（typing）— C++17 / Qt Widgets / CMake
# 跨平台 Makefile：自动识别 OS、探测 Qt / 生成器，一条 `make` 即可编译。
# 详细说明见 Docs/构建与打包.md。
#
# 常用用法：
#   make                                  # 配置 + 编译（Release）
#   make run                              # 运行程序
#   make selftest                         # 无界面自检（退出码 0 为通过）
#   make dist                             # 打包分发（自动部署 Qt 运行库）
#   make clean / make distclean           # 清理
#
# 可选覆盖（一般不需要）：
#   make QT_PREFIX=/path/to/Qt            # 指定 Qt 前缀
#   make GENERATOR=Ninja                  # 指定 CMake 生成器
#   make CONFIGURE_FLAGS="..."            # 追加 CMake 配置参数
# ============================================================================

# ---------- 可覆盖变量 ----------
CMAKE           ?= cmake
BUILD_TYPE      ?= Release
BUILD_DIR       ?= build
# 留空则自动探测；命令行可覆盖，例如：make QT_PREFIX=/opt/Qt/6.8.3/gcc_64
QT_PREFIX       ?=
# 留空则：有 ninja 用 Ninja，否则用 CMake 默认生成器
GENERATOR       ?=
# 追加到自动探测结果之后（同名参数以这边为准）
CONFIGURE_FLAGS ?=

# ---------- 平台识别 ----------
ifeq ($(OS),Windows_NT)
  HOST_OS := windows
  PYTHON  := python
  EXE     := $(BUILD_DIR)/typing.exe
else
  UNAME_S := $(shell uname -s 2>/dev/null)
  ifeq ($(UNAME_S),Darwin)
    HOST_OS := macos
  else
    HOST_OS := linux
  endif
  PYTHON := python3
  EXE    := $(BUILD_DIR)/typing
endif

# ---------- 自动选择生成器（有 ninja 则优先） ----------
ifeq ($(GENERATOR),)
  ifneq ($(shell command -v ninja 2>/dev/null),)
    GENERATOR := Ninja
  endif
endif

# ---------- 自动探测 Qt（QT_PREFIX 未指定时） ----------
# Linux   : 优先官方安装器 ~/Qt|/opt/Qt/*/gcc_64（取最新），否则 /usr 系统包
# macOS   : Homebrew qt@6/qt，或官方安装器 macos|clang_64|gcc_64
# Windows : %USERPROFILE%/Qt 或 C:/Qt；有 cl.exe 优先 msvc*_64，否则 mingw_64
ifeq ($(QT_PREFIX),)
  ifeq ($(HOST_OS),linux)
    QT_PREFIX := $(shell \
      newest=$$(ls -d "$$HOME"/Qt/*/gcc_64 /opt/Qt/*/gcc_64 2>/dev/null | sort -V | tail -n 1); \
      if [ -n "$$newest" ] && [ -f "$$newest/lib/cmake/Qt6/Qt6Config.cmake" ]; then \
        echo "$$newest"; exit 0; \
      fi; \
      for p in /usr /usr/local; do \
        if [ -f "$$p/lib/cmake/Qt6/Qt6Config.cmake" ]; then echo "$$p"; exit 0; fi; \
      done; \
      true)
  else ifeq ($(HOST_OS),macos)
    QT_PREFIX := $(shell \
      for p in /opt/homebrew/opt/qt@6 /opt/homebrew/opt/qt \
               /usr/local/opt/qt@6 /usr/local/opt/qt; do \
        if [ -d "$$p/lib/cmake/Qt6" ] || [ -f "$$p/lib/cmake/Qt6/Qt6Config.cmake" ]; then \
          echo "$$p"; exit 0; \
        fi; \
      done; \
      newest=$$(ls -d "$$HOME"/Qt/*/macos "$$HOME"/Qt/*/clang_64 "$$HOME"/Qt/*/gcc_64 \
                    /opt/Qt/*/macos /opt/Qt/*/clang_64 /opt/Qt/*/gcc_64 \
                    2>/dev/null | sort -V | tail -n 1); \
      if [ -n "$$newest" ] && [ -f "$$newest/lib/cmake/Qt6/Qt6Config.cmake" ]; then \
        echo "$$newest"; exit 0; \
      fi; \
      true)
  else ifeq ($(HOST_OS),windows)
    QT_PREFIX := $(shell \
      roots=; \
      for r in "$$USERPROFILE/Qt" "C:/Qt" "/c/Qt"; do \
        [ -d "$$r" ] && roots="$$roots $$r"; \
      done; \
      if command -v cl >/dev/null 2>&1; then \
        patterns='msvc*_64 mingw_64 llvm-mingw_64'; \
      else \
        patterns='mingw_64 llvm-mingw_64 msvc*_64'; \
      fi; \
      for kit in $$patterns; do \
        newest=; \
        for root in $$roots; do \
          for d in $$(ls -d "$$root"/$$kit "$$root"/*/$$kit 2>/dev/null); do \
            [ -f "$$d/lib/cmake/Qt6/Qt6Config.cmake" ] || continue; \
            newest="$$newest $$d"; \
          done; \
        done; \
        pick=$$(echo $$newest | tr ' ' '\n' | grep -v '^$$' | sort -V | tail -n 1); \
        if [ -n "$$pick" ]; then echo "$$pick"; exit 0; fi; \
      done; \
      true)
  endif
endif

# ---------- 组装 CMake 配置参数 ----------
AUTO_FLAGS :=
ifneq ($(GENERATOR),)
  AUTO_FLAGS += -G "$(GENERATOR)"
endif
# 若调用方已在 CONFIGURE_FLAGS 里写了 CMAKE_PREFIX_PATH，则不再重复加
ifeq ($(filter -DCMAKE_PREFIX_PATH%,$(CONFIGURE_FLAGS)),)
  ifneq ($(QT_PREFIX),)
    AUTO_FLAGS += -DCMAKE_PREFIX_PATH="$(QT_PREFIX)"
  endif
endif

# ---------- 目标 ----------
.PHONY: all help configure build run selftest check dist clean distclean show-config

all: build

help:
	@echo "打字练习（typing）常用目标:"
	@echo "  make                    = make build（配置 + 编译）"
	@echo "  make configure          = 仅配置"
	@echo "  make build              = 配置并编译"
	@echo "  make run                = 运行程序"
	@echo "  make selftest           = 无界面自检（退出码 0 为通过）"
	@echo "  make dist               = 打包分发（Windows/macOS 自动部署 Qt 运行库）"
	@echo "  make show-config        = 显示自动探测到的平台 / Qt / 生成器"
	@echo "  make clean              = 删除构建目录 $(BUILD_DIR)"
	@echo "  make distclean          = 删除构建目录与分发目录 dist"
	@echo ""
	@echo "一般直接 make 即可；可选覆盖:"
	@echo "  make QT_PREFIX=/path/to/Qt"
	@echo "  make GENERATOR=Ninja"
	@echo "  make CONFIGURE_FLAGS=\"-DCMAKE_BUILD_TYPE=Debug\""

show-config:
	@echo "HOST_OS=$(HOST_OS)"
	@echo "QT_PREFIX=$(QT_PREFIX)"
	@echo "GENERATOR=$(GENERATOR)"
	@echo "BUILD_TYPE=$(BUILD_TYPE)"
	@echo "BUILD_DIR=$(BUILD_DIR)"
	@echo "EXE=$(EXE)"
	@echo "AUTO_FLAGS=$(AUTO_FLAGS)"
	@echo "CONFIGURE_FLAGS=$(CONFIGURE_FLAGS)"

configure:
	@if [ -z "$(QT_PREFIX)" ] && [ -z "$(filter -DCMAKE_PREFIX_PATH%,$(CONFIGURE_FLAGS))" ]; then \
	  echo "注意: 未探测到 Qt6，将依赖 CMake 默认搜索路径（系统包或 CMAKE_PREFIX_PATH 环境变量）。"; \
	  echo "      若配置失败，请: make QT_PREFIX=/path/to/Qt  或安装 qt6-base-dev / Qt online installer。"; \
	fi
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(AUTO_FLAGS) $(CONFIGURE_FLAGS)

build: configure
	$(CMAKE) --build $(BUILD_DIR) -j

run: build
	"$(EXE)"

selftest: build
	"$(EXE)" --selftest

check: selftest

dist: build
	$(PYTHON) tools/package.py --exe "$(EXE)" --out dist

clean:
	$(CMAKE) -E remove_directory $(BUILD_DIR)

distclean: clean
	$(CMAKE) -E remove_directory dist
