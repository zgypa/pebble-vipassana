#!/usr/bin/env python3
"""Post-build script to ensure configurable capability is in capabilities array
and rebuild the PBW package with the corrected appinfo.json.

This fixes an issue where the Pebble SDK doesn't always add 'configurable' to
the capabilities array even when configurable=true is set in package.json.
This is required for the Settings button to appear in the Pebble mobile app.
"""

import json
import os
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path


def fix_appinfo_capabilities(appinfo_path):
    """Add 'configurable' to capabilities array if configurable is true."""
    try:
        with open(appinfo_path, "r") as f:
            appinfo = json.load(f)

        # Check if app is marked as configurable
        is_configurable = appinfo.get("configurable", False)
        capabilities = appinfo.get("capabilities", [])

        if is_configurable and "configurable" not in capabilities:
            print(f"Adding 'configurable' to capabilities array in {appinfo_path}")
            capabilities.append("configurable")
            appinfo["capabilities"] = capabilities

            # Write back with pretty formatting
            with open(appinfo_path, "w") as f:
                json.dump(appinfo, f, indent=4)

            print("✓ Fixed capabilities array")
            return True, appinfo
        elif is_configurable and "configurable" in capabilities:
            print(f"✓ Capabilities array already correct in {appinfo_path}")
            return False, appinfo
        else:
            print(f"App is not configurable, no changes needed")
            return False, appinfo

    except Exception as e:
        print(f"Error fixing appinfo.json: {e}", file=sys.stderr)
        sys.exit(1)


def rebuild_pbw(pbw_path, new_appinfo):
    """Rebuild the PBW file with the updated appinfo.json."""
    try:
        print(f"\nRebuilding {pbw_path.name} with corrected appinfo.json...")

        # Create a temporary directory for extraction
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)

            # Extract the PBW
            with zipfile.ZipFile(pbw_path, "r") as zip_ref:
                zip_ref.extractall(temp_path)

            # Update the appinfo.json in the extracted directory
            appinfo_in_zip = temp_path / "appinfo.json"
            with open(appinfo_in_zip, "w") as f:
                json.dump(new_appinfo, f, indent=4)

            # Create a new PBW with the updated appinfo.json
            temp_pbw = temp_path / "temp.pbw"
            with zipfile.ZipFile(temp_pbw, "w", zipfile.ZIP_DEFLATED) as zip_out:
                # Walk through all files and add them to the new zip
                for root, dirs, files in os.walk(temp_path):
                    for file in files:
                        if file == "temp.pbw":
                            continue
                        file_path = Path(root) / file
                        arcname = file_path.relative_to(temp_path)
                        zip_out.write(file_path, arcname)

            # Replace the original PBW
            shutil.move(temp_pbw, pbw_path)

        print(f"✓ Successfully rebuilt {pbw_path.name}")
        return True

    except Exception as e:
        print(f"Error rebuilding PBW: {e}", file=sys.stderr)
        return False


def main():
    # Find the build directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    build_dir = project_root / "build"
    appinfo_path = build_dir / "appinfo.json"
    pbw_path = build_dir / "pebble-vipassana.pbw"

    if not appinfo_path.exists():
        print(f"Error: {appinfo_path} not found. Run build first.", file=sys.stderr)
        sys.exit(1)

    if not pbw_path.exists():
        print(f"Error: {pbw_path} not found. Run build first.", file=sys.stderr)
        sys.exit(1)

    # Fix the appinfo.json
    changed, new_appinfo = fix_appinfo_capabilities(appinfo_path)

    # Rebuild the PBW if we made changes
    if changed:
        success = rebuild_pbw(pbw_path, new_appinfo)
        sys.exit(0 if success else 1)
    else:
        print("No changes needed to PBW file")
        sys.exit(0)


if __name__ == "__main__":
    main()
