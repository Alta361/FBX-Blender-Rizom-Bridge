![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Blender](https://img.shields.io/badge/Blender-4.2-orange)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)

# FBX-Blender-Rizom-Bridge

Blender ↔ RizomUV Addon 

Preserves UV groups and metadata through the Blender editing pipeline.

Important: UV groups are only preserved for objects whose topology wasn't changed in Blender. 

In some cases, UV edges may be merged during the Blender → RizomUV export cycle.


---

## Why This Addon Exists

While working on a large project with 200+ objects in RizomUV, I dididn't notice 2 objects had wrong topology. By time i realized, I had already created extensive UV group organization in RizomUV.

The problem: The RizomUV-Blender bridge does not preserve UV groups when using FBX format, and RizomUV doesn't allow changing to USD format mid-project (the only format that properly supports group data).

The solution: This addon extracts and preserves RizomUV group data through a smart caching system, allowing you to safely edit meshes in Blender without losing your UV organization work.

## Features

✓ Import from RizomUV - Extract FBX with full metadata (groups, seams)

✓ Export to RizomUV - Inject UV groups back into FBX automatically

✓ Cache Management - Smart caching system for multiple projects

✓ Selected Objects Export - Export single or multiple meshes

✓ Blender 4.2+ Compatible - Tested and optimized for latest Blender

✓ File Menu Integration - Standard Blender import/export workflo


## Installation
Method 1: Install from Blender (Recommended)
Download the addon .zip file

In Blender: Edit → Preferences → Add-ons

Click "Install from File" → Select the .zip

Search "RizomUV" and enable the addon

Restart Blender

Method 2: Manual Installation
Download the addon folder

Place in: Blender/scripts/addons/Rizom_bridge/

Enable in Blender: Edit → Preferences → Add-ons → Search "RizomUV"

Restart Blender

### Workflow

1. RizomUV → Export FBX
2. Blender → File > Import > RizomUV Bridge
   (Extracts UV groups to cache)
3. Edit your mesh in Blender
4. Blender → File > Export > RizomUV Bridge
   (Injects UV groups from cache)
5. RizomUV → Import FBX

Important: UV groups are only preserved for objects whose topology wasn't changed in Blender.

### UI Panel

Access via: View 3D > RizomUV Panel (sidebar)

- **1. Import FBX** - Import and extract UV data
- **2. Export + Inject** - Export and restore UV groups
- **Cache List** - Manage cached files
- **Clear All Caches** - Remove all cached data


## Known Limitations

⚠️ UV Edge Merging (FBX Format Issue)

In some cases, UV edges may be merged during the Blender → RizomUV export cycle. Based on my observation, this appears to be related to how the FBX format stores UV coordinates (per-vertex rather than per-face-corner, unlike Blender's internal structure).

What this means:

When exporting from Blender, vertices with multiple UV coordinates may be merged

This can result in UV seams being lost in random cases

This issue affects all Blender ↔ RizomUV workflows, not just this addon

Note: This is my interpretation of the issue based on testing. The exact cause may be more complex and involve both Blender's FBX exporter behavior and the FBX format specification.


## License
This addon is provided free for personal and commercial use under the MIT License.

See LICENSE file for full details.

---

## Support & Contact

**Creator:** [Alta]  

**Website:** [https://www.artstation.com/alta36]

**Email:** [Patryk.Dylus@gmail.com] 

**GitHub:** [https://github.com/Alta361/FBX-Blender-Rizom-Bridge]

**Version:** 1.0  

**Last Updated:** October 2025

### Tested Configurations

✓ Blender 4.2 (Windows 64-bit)  
✓ RizomUV 2024  
✓ Windows 11   

### Not Tested

- macOS or Linux
- Other Blender and RizomUV versions

---

## Developer Notes

### File Formats

**Cache Format (.dat):**
- Binary format
- Contains extracted RizomUV metadata
- One cache per imported FBX
- Stored in `.cache/` folder

---

## Support & Contribution

**Report Issues:**
- Create detailed bug reports
- Include Blender version, OS, and steps to reproduce
- Attach small test files if possible

**Want to Contribute?**
- Fork the repository
- Make improvements
- Submit pull requests

---

---

## Credits

**Technologies:**
- Blender Python API
- Autodesk FBX SDK 2020
- Windows C++

---

## Disclaimer

This addon is a community tool, not officially affiliated with RizomUV or Blender Foundation. Use at your own risk. Always back up your files before using with valuable projects.
