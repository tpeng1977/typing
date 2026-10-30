#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""跨平台打包脚本：把构建产物复制到 dist/ 并在需要时带上 Qt 运行库。

Windows 下优先使用 windeployqt；找不到时退化为手动部署
（Qt6Core/Gui/Widgets.dll + platforms/qwindows.dll + MinGW 运行库），
与工程验证过的部署方式一致。

用法:
    python tools/package.py --exe build/typing.exe --out dist
    python tools/package.py --exe build/typing.exe --out dist \
        --qt C:/Qt/6.5.3/mingw_64 --compiler C:/Qt/Tools/mingw1120_64/bin
"""
import argparse
import os
import shutil
import subprocess
import sys


def find_in_path(name, extra_dirs=()):
    """按 PATH（及额外目录）查找文件。

    注意：不能直接用 shutil.which——Windows 下其对 .dll 会因
    os.access(X_OK) 检查失败而返回 None（X_OK 仅对 .exe 等为真）。
    """
    dirs = list(extra_dirs)
    for d in os.environ.get("PATH", "").split(os.pathsep):
        if d:
            dirs.append(d)
    for d in dirs:
        d = d.strip('"').strip()
        if not d:
            continue
        p = os.path.join(d, name)
        if os.path.isfile(p):
            return p
    return None


def find_windeployqt(qt_prefix):
    cands = []
    if qt_prefix:
        cands.append(os.path.join(qt_prefix, "bin", "windeployqt.exe"))
    wd = shutil.which("windeployqt")
    if wd:
        cands.append(wd)
    for c in cands:
        if c and os.path.isfile(c):
            return c
    return None


def deploy_win(exe, out, qt_prefix, compiler):
    wd = find_windeployqt(qt_prefix)
    if wd:
        print("使用 windeployqt: %s" % wd)
        env = dict(os.environ)
        if qt_prefix:
            env["PATH"] = os.path.join(qt_prefix, "bin") + os.pathsep + env.get("PATH", "")
        if compiler:
            env["PATH"] = compiler + os.pathsep + env.get("PATH", "")
        r = subprocess.run([wd, "--release", "--dir", out, exe], env=env)
        if r.returncode == 0:
            return
        print("警告: windeployqt 失败（退出码 %d），回退到手动部署" % r.returncode)

    print("手动部署（Qt6Core/Gui/Widgets + 平台插件 + MinGW 运行库）")
    qt_bin = os.path.join(qt_prefix, "bin") if qt_prefix else None
    for dll in ("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll"):
        src = find_in_path(dll, (qt_bin,) if qt_bin else ())
        if src:
            shutil.copy2(src, os.path.join(out, dll))
        else:
            print("警告: 未找到 %s（若程序能独立运行可忽略）" % dll)

    # 平台插件：优先用显式 --qt；否则从找到的 Qt6Core.dll 位置推导 Qt 前缀
    qt_root = qt_prefix
    core = find_in_path("Qt6Core.dll", (qt_bin,) if qt_bin else ())
    if not qt_root and core:
        qt_root = os.path.dirname(os.path.dirname(core))
    plat = os.path.join(qt_root or "", "plugins", "platforms", "qwindows.dll")
    if os.path.isfile(plat):
        d = os.path.join(out, "platforms")
        os.makedirs(d, exist_ok=True)
        shutil.copy2(plat, os.path.join(d, "qwindows.dll"))
    else:
        print("警告: 未找到 platforms/qwindows.dll")

    for dll in ("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"):
        src = find_in_path(dll, (compiler,) if compiler else ())
        if src:
            shutil.copy2(src, os.path.join(out, dll))
        else:
            print("警告: 未找到 %s" % dll)


def deploy_mac(exe, out, qt_prefix):
    app = exe
    if app.endswith(".app"):
        macdeployqt = shutil.which("macdeployqt")
        if not macdeployqt and qt_prefix:
            macdeployqt = os.path.join(qt_prefix, "bin", "macdeployqt")
        if macdeployqt and os.path.isfile(macdeployqt):
            subprocess.run([macdeployqt, app, "-always-overwrite"])
            print("已用 macdeployqt 处理应用包")
        else:
            print("警告: 未找到 macdeployqt，请在装有 Qt 的机器上运行 macdeployqt 后再分发")
    else:
        print("提示: 请先构建出 typing.app 再打包")


def main():
    ap = argparse.ArgumentParser(description="typing 打包脚本")
    ap.add_argument("--exe", required=True, help="构建产物路径")
    ap.add_argument("--out", default="dist", help="输出目录（默认 dist）")
    ap.add_argument("--qt", default=None, help="Qt 前缀目录，如 C:/Qt/6.5.3/mingw_64")
    ap.add_argument("--compiler", default=None, help="编译器 bin 目录（Windows 手动部署时查找 MinGW 运行库）")
    args = ap.parse_args()

    exe = os.path.abspath(args.exe)
    out = os.path.abspath(args.out)
    if not os.path.isfile(exe):
        sys.exit("错误: 找不到可执行文件 %s，请先 make build" % exe)
    os.makedirs(out, exist_ok=True)
    shutil.copy2(exe, os.path.join(out, os.path.basename(exe)))

    if sys.platform.startswith("win"):
        deploy_win(exe, out, args.qt, args.compiler)
    elif sys.platform == "darwin":
        deploy_mac(exe, out, args.qt)
    else:
        print("Linux/其他平台：假定 Qt 由系统软件源提供，已复制可执行文件到 %s" % out)
    print("打包完成 -> %s" % out)


if __name__ == "__main__":
    main()
