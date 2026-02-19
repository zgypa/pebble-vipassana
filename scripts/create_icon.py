#!/usr/bin/env python3
"""Create Dhamma Wheel icon as SVG for Pebble watchface."""

import math


def create_dhamma_wheel_svg(size=100):
    """Create a Dhamma Wheel (Dharmachakra) as SVG."""
    center = size / 2
    outer_radius = size * 0.45
    inner_radius = size * 0.15
    hub_radius = size * 0.10
    num_spokes = 8
    # Thicker lines for small icon visibility - 6% of size for 25px = 1.5px
    stroke_width = size * 0.06

    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg width="{size}" height="{size}" viewBox="0 0 {size} {size}" xmlns="http://www.w3.org/2000/svg">
  <rect width="{size}" height="{size}" fill="white"/>
  <g stroke="black" stroke-width="{stroke_width}" fill="none" stroke-linecap="round">
'''

    # Outer rim
    svg += f'    <circle cx="{center}" cy="{center}" r="{outer_radius}"/>\n'

    # Inner rim
    svg += f'    <circle cx="{center}" cy="{center}" r="{inner_radius}"/>\n'

    # Spokes
    for i in range(num_spokes):
        angle = 2 * math.pi * i / num_spokes
        x1 = center + inner_radius * math.cos(angle)
        y1 = center + inner_radius * math.sin(angle)
        x2 = center + outer_radius * math.cos(angle)
        y2 = center + outer_radius * math.sin(angle)
        svg += f'    <line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}"/>\n'

    # Hub (center circle) - filled
    svg += f'    <circle cx="{center}" cy="{center}" r="{hub_radius}" fill="black"/>\n'

    svg += """  </g>
</svg>"""

    return svg


if __name__ == "__main__":
    # Create 25x25 icon for Pebble (standard size)
    svg_25 = create_dhamma_wheel_svg(25)
    with open("resources/icon_25.svg", "w") as f:
        f.write(svg_25)
    print("Created resources/icon_25.svg")

    # Also create larger version for reference
    svg_100 = create_dhamma_wheel_svg(100)
    with open("resources/icon_100.svg", "w") as f:
        f.write(svg_100)
    print("Created resources/icon_100.svg")
