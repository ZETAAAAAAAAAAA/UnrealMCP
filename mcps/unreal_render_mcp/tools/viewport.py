"""
Viewport Tools for Unreal Render MCP

Viewport capture and camera control tools.
"""

from typing import Dict, Any, Optional
import logging

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from common import send_command, with_unreal_connection

logger = logging.getLogger("UnrealRenderMCP")


@with_unreal_connection
def get_viewport_screenshot(
    output_path: str,
    format: str = "png",
    quality: int = 85,
    include_ui: bool = False,
    output_mode: str = "file"
) -> Dict[str, Any]:
    """
    Capture a screenshot of the current viewport and save as image file.
    
    Args:
        output_path: Full path where to save the screenshot
        format: Image format - "png", "jpg", or "bmp"
        quality: JPEG quality 1-100 (only for jpg)
        include_ui: Whether to include editor UI
        output_mode: Output mode - "file" (save to disk), "rgba" (return RGBA base64), "rgb" (return RGB base64)
    """
    return send_command("get_viewport_screenshot", {
        "output_path": output_path,
        "format": format,
        "quality": quality,
        "include_ui": include_ui,
        "output_mode": output_mode
    })


@with_unreal_connection
def set_viewport_camera(
    location: Optional[Dict[str, float]] = None,
    rotation: Optional[Dict[str, float]] = None,
    fov: Optional[float] = None,
    ortho_width: Optional[float] = None,
    perspective: bool = True
) -> Dict[str, Any]:
    """
    Set the editor viewport camera position and rotation.
    
    Args:
        location: Camera position {x, y, z} in world space
        rotation: Camera rotation {pitch, yaw, roll} in degrees
        fov: Field of view for perspective camera (default: 90)
        ortho_width: Ortho width for orthographic camera
        perspective: True for perspective, False for orthographic
    
    Example:
        set_viewport_camera(
            location={"x": 100, "y": 0, "z": 50},
            rotation={"pitch": -10, "yaw": 0, "roll": 0},
            fov=60
        )
    """
    params = {"perspective": perspective}
    
    if location is not None:
        params["location"] = location
    if rotation is not None:
        params["rotation"] = rotation
    if fov is not None:
        params["fov"] = fov
    if ortho_width is not None:
        params["ortho_width"] = ortho_width
    
    return send_command("set_viewport_camera", params)


@with_unreal_connection
def get_viewport_camera() -> Dict[str, Any]:
    """
    Get the current editor viewport camera position and rotation.
    
    Returns:
        Camera info including location, rotation, fov, and perspective mode
    """
    return send_command("get_viewport_camera", {})
