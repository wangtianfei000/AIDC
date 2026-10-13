from __future__ import annotations

from collections.abc import Callable

from cli.aidc import main as cli_main, parser


def _target(read: Callable[[str], str], prompt: str) -> str:
    """Read a path typed or dragged into cmd.exe and remove shell quotes."""
    return read(prompt).strip().strip('"')


def _run(arguments: list[str], read: Callable[[str], str]) -> None:
    cli_main(arguments)
    read("\n按回车键返回主菜单……")


def main(input_fn: Callable[[str], str] | None = None) -> int:
    read = input_fn or input

    while True:
        print("\n" + "=" * 60)
        print(" AIDC Engineering Tools")
        print(" 面向 Codex、Cursor 和终端的机器人工程工具")
        print("=" * 60)
        print("  [1] 打开 SysML 模型浏览器")
        print("  [2] 打开三维模型浏览器")
        print("  [3] 打开 Modelica 模型浏览器")
        print("  [4] 检查 AIDC 环境")
        print("  [5] 查看依赖状态")
        print("  [6] 查看命令帮助")
        print("  [0] 退出")
        choice = read("\n请选择：").strip()

        if choice == "0":
            return 0
        if choice in {"1", "2", "3"}:
            labels = {
                "1": ("sysml", "请输入 .sysml 文件路径（可将文件拖到窗口）："),
                "2": ("model3d", "请输入三维模型工程目录（可将目录拖到窗口）："),
                "3": ("modelica", "请输入 Modelica 工程目录（可将目录拖到窗口）："),
            }
            domain, prompt = labels[choice]
            target = _target(read, prompt)
            if target:
                _run([domain, "open", target], read)
            continue
        if choice == "4":
            _run(["doctor"], read)
            continue
        if choice == "5":
            _run(["deps", "status"], read)
            continue
        if choice == "6":
            parser().print_help()
            print("\n常用终端示例：")
            print(r'  bin\aidc.cmd doctor')
            print(r'  bin\aidc.cmd sysml open "D:\project\architecture.sysml"')
            print(r'  bin\aidc.cmd model3d open "D:\project\model3d"')
            print(r'  bin\aidc.cmd modelica open "D:\project\modelica"')
            read("\n按回车键返回主菜单……")
            continue
        print("请输入 0 到 6 之间的数字。")
