#!/bin/bash
# TurtleClass Cloudflare Tunnel Deployment Script
# 自动化部署脚本：安装 cloudflared、配置隧道、启动服务

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 配置变量
TUNNEL_NAME="turtleclass-prod"
TURTLECLASS_PORT=8080
CLOUDFLARED_CONFIG_DIR="/etc/cloudflared"
TURTLECLASS_CONFIG_DIR="/etc/turtleclass"
LOG_DIR="/var/log/turtleclass"

echo -e "${GREEN}=== TurtleClass Cloudflare Tunnel Deployment ===${NC}"

# 检查是否以 root 运行
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Error: Please run as root (sudo)$NC"
    exit 1
fi

# 创建必要目录
echo -e "${YELLOW}Creating directories...${NC}"
mkdir -p "$CLOUDFLARED_CONFIG_DIR"
mkdir -p "$TURTLECLASS_CONFIG_DIR"
mkdir -p "$LOG_DIR"
mkdir -p /var/lib/turtleclass

# 检测操作系统
if [ -f /etc/debian_version ]; then
    OS="debian"
elif [ -f /etc/redhat-release ]; then
    OS="redhat"
else
    echo -e "${RED}Unsupported OS${NC}"
    exit 1
fi

# 安装 cloudflared
echo -e "${YELLOW}Installing cloudflared...${NC}"
if [ "$OS" = "debian" ]; then
    curl -fsSL https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64.deb -o /tmp/cloudflared.deb
    dpkg -i /tmp/cloudflared.deb
    rm /tmp/cloudflared.deb
elif [ "$OS" = "redhat" ]; then
    curl -fsSL https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64.rpm -o /tmp/cloudflared.rpm
    dnf install -y /tmp/cloudflared.rpm
    rm /tmp/cloudflared.rpm
fi

echo -e "${GREEN}cloudflared installed successfully${NC}"

# 创建隧道（如果不存在）
echo -e "${YELLOW}Checking tunnel existence...${NC}"
if cloudflared tunnel list | grep -q "$TUNNEL_NAME"; then
    echo -e "${GREEN}Tunnel $TUNNEL_NAME already exists${NC}"
else
    echo -e "${YELLOW}Creating new tunnel: $TUNNEL_NAME${NC}"
    cloudflared tunnel create "$TUNNEL_NAME"
fi

# 获取隧道凭证
echo -e "${YELLOW}Downloading tunnel credentials...${NC}"
cloudflared tunnel credentials "$TUNNEL_NAME" -o "$CLOUDFLARED_CONFIG_DIR/credentials.json"
chmod 600 "$CLOUDFLARED_CONFIG_DIR/credentials.json"

# 复制配置文件
echo -e "${YELLOW}Copying configuration files...${NC}"
cp /workspace/deploy/cloudflare/tunnel.yml "$TURTLECLASS_CONFIG_DIR/tunnel.yml"

# 更新配置文件中的隧道名称
sed -i "s/turtleclass-prod/$TUNNEL_NAME/g" "$TURTLECLASS_CONFIG_DIR/tunnel.yml"

# 路由 DNS（需要用户确认域名）
echo -e "${YELLOW}Setting up DNS routes...${NC}"
read -p "Enter your domain for API (e.g., api.turtleclass.example.com): " API_DOMAIN
read -p "Enter your domain for Health check (e.g., health.turtleclass.example.com): " HEALTH_DOMAIN

cloudflared tunnel route dns "$TUNNEL_NAME" "$API_DOMAIN"
cloudflared tunnel route dns "$TUNNEL_NAME" "$HEALTH_DOMAIN"

# 创建 systemd 服务文件
echo -e "${YELLOW}Creating systemd service...${NC}"
cat > /etc/systemd/system/cloudflared-turtleclass.service << EOF
[Unit]
Description=Cloudflare Tunnel for TurtleClass
After=network.target turtleclass-server.service

[Service]
Type=simple
User=root
ExecStart=/usr/local/bin/cloudflared tunnel --config $TURTLECLASS_CONFIG_DIR/tunnel.yml run $TUNNEL_NAME
Restart=on-failure
RestartSec=5s
Environment=TURTLECLASS_DATA_DIR=/var/lib/turtleclass

# 日志
StandardOutput=journal
StandardError=journal
SyslogIdentifier=cloudflared-turtleclass

[Install]
WantedBy=multi-user.target
EOF

# 重新加载 systemd 并启用服务
echo -e "${YELLOW}Enabling and starting service...${NC}"
systemctl daemon-reload
systemctl enable cloudflared-turtleclass
systemctl start cloudflared-turtleclass

# 检查服务状态
sleep 2
if systemctl is-active --quiet cloudflared-turtleclass; then
    echo -e "${GREEN}✓ Cloudflare Tunnel service is running${NC}"
else
    echo -e "${RED}✗ Service failed to start. Check logs: journalctl -u cloudflared-turtleclass${NC}"
    exit 1
fi

# 显示隧道信息
echo -e "${GREEN}=== Deployment Complete ===${NC}"
echo ""
echo "Tunnel Name: $TUNNEL_NAME"
echo "API Endpoint: https://$API_DOMAIN"
echo "Health Check: https://$HEALTH_DOMAIN/health"
echo ""
echo "Useful commands:"
echo "  - View logs: journalctl -u cloudflared-turtleclass -f"
echo "  - Restart: systemctl restart cloudflared-turtleclass"
echo "  - Status: systemctl status cloudflared-turtleclass"
echo "  - List tunnels: cloudflared tunnel list"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "1. Update Windows client configuration with API endpoint"
echo "2. Test connectivity from client machine"
echo "3. Configure backup and monitoring"
echo ""
