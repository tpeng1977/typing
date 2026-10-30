cmake_minimum_required(VERSION 3.16)
project(typing VERSION 0.2.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动生成 moc 与 qrc 资源
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Windows：GUI 子系统（不弹出控制台窗口）
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Windows 下 WIN32 标志表示 GUI 子系统；其他平台不加
set(WIN32_FLAG "")
if(WIN32)
    set(WIN32_FLAG WIN32)
endif()

add_executable(typing ${WIN32_FLAG}
    src/main.cpp
    src/selftest.cpp
    src/imetest.cpp
    src/entest.cpp
    src/article.h
    src/article.cpp
    src/dataloader.h
    src/dataloader.cpp
    src/bilingual.h
    src/bilingual.cpp
    src/segmentation.h
    src/segmentation.cpp
    src/justifiedlabel.h
    src/justifiedlabel.cpp
    src/justifiedinput.h
    src/justifiedinput.cpp
    src/reader.h
    src/reader.cpp
    src/practice.h
    src/practice_internal.h
    src/practice.cpp
    src/practice_lines.cpp
    src/practice_match.cpp
    src/practice_render.cpp
    src/importwidget.h
    src/importwidget.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    res/resources.qrc
)

target_link_libraries(typing PRIVATE Qt6::Widgets)

# Windows：把 .ico 写入可执行文件资源，资源管理器/任务栏显示应用图标
if(WIN32)
    set(_typing_rc "${CMAKE_CURRENT_BINARY_DIR}/typing_icon.rc")
    file(WRITE "${_typing_rc}" "IDI_ICON1 ICON \"${CMAKE_SOURCE_DIR}/res/app_icon.ico\"\n")
    target_sources(typing PRIVATE "${_typing_rc}")
endif()

# 安装
include(GNUInstallDirs)
install(TARGETS typing
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
