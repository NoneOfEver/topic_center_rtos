#!/bin/bash
# RTT Viewer Script for STM32H723
# 使用 OpenOCD 查看 SEGGER RTT 输出

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OPENOCD_CFG="${SCRIPT_DIR}/Board/dm-h723/config/openocd_dap.cfg"

# RTT 配置参数
RTT_ADDR="0x24000000"      # RAM_D1 起始地址
RTT_SIZE="0x50000"         # 搜索范围 320KB
RTT_ID="SEGGER RTT"        # RTT 控制块标识
RTT_PORT="9090"            # RTT 服务器端口
RTT_CHANNEL="0"            # RTT 通道

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

cleanup() {
    print_info "正在停止 OpenOCD..."
    if [ -n "$OPENOCD_PID" ]; then
        kill $OPENOCD_PID 2>/dev/null
    fi
    exit 0
}

trap cleanup SIGINT SIGTERM

show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -c, --config <file>   指定 OpenOCD 配置文件"
    echo "  -p, --port <port>     指定 RTT 服务器端口 (默认: 9090)"
    echo "  -a, --addr <addr>     指定 RTT 搜索起始地址 (默认: 0x24000000)"
    echo "  -s, --size <size>     指定 RTT 搜索范围 (默认: 0x50000)"
    echo "  -h, --help            显示帮助信息"
    echo ""
    echo "示例:"
    echo "  $0                    使用默认配置启动"
    echo "  $0 -p 9091            使用端口 9091"
    echo "  $0 -a 0x20000000      搜索 DTCMRAM 区域"
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--config)
            OPENOCD_CFG="$2"
            shift 2
            ;;
        -p|--port)
            RTT_PORT="$2"
            shift 2
            ;;
        -a|--addr)
            RTT_ADDR="$2"
            shift 2
            ;;
        -s|--size)
            RTT_SIZE="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
done

# 检查 OpenOCD 是否安装
if ! command -v openocd &> /dev/null; then
    print_error "未找到 openocd，请先安装"
    exit 1
fi

# 检查配置文件是否存在
if [ ! -f "$OPENOCD_CFG" ]; then
    print_error "配置文件不存在: $OPENOCD_CFG"
    exit 1
fi

print_info "RTT Viewer 配置:"
echo "  OpenOCD 配置: $OPENOCD_CFG"
echo "  RTT 地址范围: $RTT_ADDR + $RTT_SIZE"
echo "  RTT 端口: $RTT_PORT"
echo ""

# 启动 OpenOCD
print_info "正在启动 OpenOCD..."
openocd -f "$OPENOCD_CFG" \
    -c "init" \
    -c "rtt setup $RTT_ADDR $RTT_SIZE \"$RTT_ID\"" \
    -c "rtt start" \
    -c "rtt server start $RTT_PORT $RTT_CHANNEL" &

OPENOCD_PID=$!

# 等待 OpenOCD 启动并找到 RTT 控制块
print_info "等待 OpenOCD 初始化..."
MAX_WAIT=20
WAITED=0
while [ $WAITED -lt $MAX_WAIT ]; do
    # 检查 OpenOCD 是否还在运行
    if ! kill -0 $OPENOCD_PID 2>/dev/null; then
        print_error "OpenOCD 启动失败"
        exit 1
    fi
    
    # 检查 RTT 端口是否已开启
    if nc -z localhost $RTT_PORT 2>/dev/null; then
        break
    fi
    
    sleep 1
    WAITED=$((WAITED + 1))
done

if [ $WAITED -ge $MAX_WAIT ]; then
    print_error "等待 RTT 服务器超时"
    cleanup
    exit 1
fi

# 额外等待确保 RTT 完全就绪
sleep 1

print_info "OpenOCD 已启动 (PID: $OPENOCD_PID)"
print_info "正在连接 RTT 服务器 (端口 $RTT_PORT)..."
print_info "按 Ctrl+C 退出"
echo ""
echo "========== RTT 输出 =========="
echo ""

# 连接 RTT 服务器并显示输出，自动重连
while kill -0 $OPENOCD_PID 2>/dev/null; do
    nc localhost $RTT_PORT
    # 如果 nc 断开但 OpenOCD 还在运行，尝试重连
    if kill -0 $OPENOCD_PID 2>/dev/null; then
        print_warn "连接断开，正在重连..."
        sleep 1
    fi
done

# 脚本结束时清理
cleanup
