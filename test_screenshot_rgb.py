"""测试视口截图RGB输出功能"""
import base64
import sys
sys.path.insert(0, r"d:\CodeBuddy\rendering-mcp\mcps\unreal_render_mcp\tools")

from viewport import get_viewport_screenshot

# 测试1: RGB模式 - 返回像素数据
print("=" * 50)
print("测试1: RGB模式截图")
result = get_viewport_screenshot(
    output_path=r"d:\CodeBuddy\rendering-mcp\output\test_rgb.png",
    output_mode="rgb",
    format="png"
)

if result.get("success"):
    print(f"截图成功!")
    print(f"图像尺寸: {result.get('width')} x {result.get('height')}")
    
    # 解码RGB数据
    pixel_data_b64 = result.get("pixel_data")
    if pixel_data_b64:
        raw_bytes = base64.b64decode(pixel_data_b64)
        width = result.get('width', 0)
        height = result.get('height', 0)
        
        # 计算中心像素索引 (RGB = 3 bytes per pixel)
        center_idx = ((height // 2) * width + (width // 2)) * 3
        
        if center_idx + 2 < len(raw_bytes):
            r = raw_bytes[center_idx]
            g = raw_bytes[center_idx + 1]
            b = raw_bytes[center_idx + 2]
            print(f"中心像素 RGB: ({r}, {g}, {b})")
            print(f"中心像素灰度值: {r/255:.3f}, {g/255:.3f}, {b/255:.3f}")
else:
    print(f"截图失败: {result.get('error', '未知错误')}")

# 测试2: 传统file模式
print("\n" + "=" * 50)
print("测试2: File模式截图")
result2 = get_viewport_screenshot(
    output_path=r"d:\CodeBuddy\rendering-mcp\output\test_file.png",
    output_mode="file",
    format="png"
)

if result2.get("success"):
    print(f"文件已保存: {result2.get('output_path')}")
else:
    print(f"截图失败: {result2.get('error', '未知错误')}")

print("\n测试完成!")
