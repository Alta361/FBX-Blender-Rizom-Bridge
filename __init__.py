"""
RizomUV Bridge Pro - v2.1.2 BLENDER 4.2 FIX
PRODUCTION READY

- FIXED: Removed use_ascii parameter (Blender 4.2 exports Binary by default!)
- Export Selected Objects
- Groups & Cache Management
"""

import bpy
import os
import subprocess

# ============================================================================
# CACHE MANAGER
# ============================================================================

class RizomCacheManager:
    """Simple cache management"""
    
    @staticmethod
    def get_addon_cache_dir():
        addon_dir = os.path.dirname(__file__)
        cache_dir = os.path.join(addon_dir, ".cache")
        os.makedirs(cache_dir, exist_ok=True)
        return cache_dir
    
    @staticmethod
    def get_cache_path(fbx_filename):
        cache_dir = RizomCacheManager.get_addon_cache_dir()
        base_name = os.path.splitext(fbx_filename)[0]
        return os.path.join(cache_dir, f"{base_name}.dat")
    
    @staticmethod
    def list_all_caches():
        cache_dir = RizomCacheManager.get_addon_cache_dir()
        if not os.path.exists(cache_dir):
            return []
        
        cache_files = []
        for filename in os.listdir(cache_dir):
            if filename.endswith(".dat"):
                filepath = os.path.join(cache_dir, filename)
                size = os.path.getsize(filepath)
                cache_files.append({
                    "filename": filename,
                    "fbx_name": filename.replace(".dat", ".fbx"),
                    "path": filepath,
                    "size_kb": size / 1024
                })
        
        return sorted(cache_files, key=lambda x: x["filename"])
    
    @staticmethod
    def delete_cache(cache_filename):
        cache_dir = RizomCacheManager.get_addon_cache_dir()
        cache_path = os.path.join(cache_dir, cache_filename)
        if os.path.exists(cache_path):
            try:
                os.remove(cache_path)
                return True
            except:
                return False
        return False
    
    @staticmethod
    def clear_all_caches():
        cache_dir = RizomCacheManager.get_addon_cache_dir()
        if not os.path.exists(cache_dir):
            return 0
        
        count = 0
        for filename in os.listdir(cache_dir):
            if filename.endswith(".dat"):
                try:
                    os.remove(os.path.join(cache_dir, filename))
                    count += 1
                except:
                    pass
        return count

# ============================================================================
# IMPORT OPERATOR
# ============================================================================

class IMPORT_OT_fbx_rizom(bpy.types.Operator):
    """Import FBX (RizomUV Bridge)"""
    bl_idname = "import_scene.fbx_rizom"
    bl_label = "RizomUV Bridge (.fbx)"
    bl_options = {'PRESET', 'UNDO', 'REGISTER'}
    
    filename_ext = ".fbx"
    filter_glob: bpy.props.StringProperty(default="*.fbx", options={'HIDDEN'})
    filepath: bpy.props.StringProperty(subtype="FILE_PATH")
    
    extract_rizom: bpy.props.BoolProperty(
        name="Extract RizomUV Data",
        default=True
    )
    
    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}
    
    def execute(self, context):
        fbx_path = self.filepath
        fbx_filename = os.path.basename(fbx_path)
        cache_path = RizomCacheManager.get_cache_path(fbx_filename)
        
        print(f"[RizomUV] Importing: {fbx_filename}")
        
        # Extract cache
        if self.extract_rizom:
            addon_dir = os.path.dirname(__file__)
            extractor_path = os.path.join(addon_dir, "bin", "ekstraktor.exe")
            
            if not os.path.exists(extractor_path):
                self.report({'ERROR'}, "Extractor not found!")
                return {'CANCELLED'}
            
            try:
                result = subprocess.run(
                    [extractor_path, fbx_path, cache_path],
                    capture_output=True,
                    text=True,
                    timeout=60
                )
                
                if result.returncode == 0:
                    self.report({'INFO'}, f"✓ Cache: {os.path.basename(cache_path)}")
                    
            except Exception as e:
                self.report({'ERROR'}, f"Extraction failed: {str(e)}")
                return {'CANCELLED'}
        
        # Import FBX
        self.report({'INFO'}, ">>> Importing FBX...")
        bpy.ops.import_scene.fbx(filepath=fbx_path)
        
        self.report({'INFO'}, "✓ Import completed!")
        return {'FINISHED'}
    
    def draw(self, context):
        layout = self.layout
        layout.prop(self, "extract_rizom")

# ============================================================================
# EXPORT OPERATOR - BLENDER 4.2 COMPATIBLE
# ============================================================================

def get_cache_enum_items(self, context):
    """Dynamic enum items for cache selection"""
    items = []
    caches = RizomCacheManager.list_all_caches()
    
    for i, cache in enumerate(caches):
        items.append((
            cache["path"],
            f"{cache['fbx_name']} ({cache['size_kb']:.0f} KB)",
            f"Use cache from {cache['fbx_name']}",
            i
        ))
    
    if not items:
        items.append(("NONE", "No caches available", "Import FBX first"))
    
    return items

class EXPORT_OT_fbx_rizom_pro(bpy.types.Operator):
    """Export FBX (RizomUV Bridge PRO)"""
    bl_idname = "export_scene.fbx_rizom_pro"
    bl_label = "RizomUV Bridge (.fbx)"
    bl_options = {'PRESET', 'REGISTER'}
    
    filename_ext = ".fbx"
    filter_glob: bpy.props.StringProperty(default="*.fbx", options={'HIDDEN'})
    filepath: bpy.props.StringProperty(subtype="FILE_PATH")
    
    inject_rizom: bpy.props.BoolProperty(
        name="Inject RizomUV Data",
        default=True
    )
    
    selected_cache: bpy.props.EnumProperty(
        name="Select Cache",
        items=get_cache_enum_items
    )
    
    use_selection: bpy.props.BoolProperty(
        name="Selected Objects",
        description="Export only selected objects",
        default=False
    )
    
    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}
    
    def execute(self, context):
        output_fbx = self.filepath
        temp_fbx = output_fbx + ".temp.fbx"
        
        print(f"[RizomUV] Exporting: {os.path.basename(output_fbx)}")
        
        # Export FBX - Blender 4.2 exports Binary by default!
        if self.use_selection:
            self.report({'INFO'}, "<<< Exporting selected objects...")
        else:
            self.report({'INFO'}, "<<< Exporting all objects...")
        
        # FIXED: No use_ascii parameter in Blender 4.2!
        bpy.ops.export_scene.fbx(
            filepath=temp_fbx,
            use_selection=self.use_selection,
            path_mode='AUTO'
        )
        
        if self.inject_rizom:
            if self.selected_cache and self.selected_cache != "NONE":
                cache_path = self.selected_cache
                self._inject(temp_fbx, output_fbx, cache_path)
            else:
                self.report({'WARNING'}, "No cache selected.")
                if os.path.exists(temp_fbx):
                    os.replace(temp_fbx, output_fbx)
        else:
            if os.path.exists(temp_fbx):
                os.replace(temp_fbx, output_fbx)
        
        self.report({'INFO'}, "✓ Export completed!")
        return {'FINISHED'}
    
    def _inject(self, temp_fbx, output_fbx, cache_path):
        """Inject cache into FBX"""
        addon_dir = os.path.dirname(__file__)
        injector_path = os.path.join(addon_dir, "bin", "injektor.exe")
        
        if not os.path.exists(injector_path):
            if os.path.exists(temp_fbx):
                os.replace(temp_fbx, output_fbx)
            return
        
        try:
            result = subprocess.run(
                [injector_path, temp_fbx, cache_path, output_fbx],
                capture_output=True,
                text=True,
                timeout=60
            )
            
            if result.returncode == 0:
                if os.path.exists(temp_fbx):
                    os.remove(temp_fbx)
                self.report({'INFO'}, "✓ RizomUV data injected!")
                print("[RizomUV] Injection successful - Binary FBX created")
            else:
                if os.path.exists(temp_fbx):
                    os.replace(temp_fbx, output_fbx)
                print(f"[RizomUV] Injection warning: {result.stderr}")
                
        except Exception as e:
            if os.path.exists(temp_fbx):
                os.replace(temp_fbx, output_fbx)
            print(f"[RizomUV] Injection error: {str(e)}")
    
    def draw(self, context):
        layout = self.layout
        
        # Selected Objects
        layout.prop(self, "use_selection")
        
        layout.separator()
        
        # Inject RizomUV Data
        layout.prop(self, "inject_rizom", text="Inject RizomUV Data")
        
        if self.inject_rizom:
            layout.separator()
            
            caches = RizomCacheManager.list_all_caches()
            
            if len(caches) > 0:
                layout.label(text="Select cache to inject:")
                layout.prop(self, "selected_cache", text="")
                
                # INFO
                layout.separator()
                box = layout.box()
                box.label(text="INFO:", icon='INFO')
                box.label(text="If you don't see the cache:")
                box.label(text="Import that FBX first to create cache")
            
            else:
                # ERROR
                box = layout.box()
                box.label(text="No cache found!", icon='ERROR')
                box.label(text="")
                box.label(text="To inject RizomUV groups:")
                box.label(text="1. File > Import > RizomUV Bridge")
                box.label(text="2. Select your FBX from RizomUV")
                box.label(text="3. This creates the cache")
                box.label(text="4. Then export will work!")

# ============================================================================
# DELETE & CLEAR CACHE
# ============================================================================

class RIZOM_OT_delete_cache(bpy.types.Operator):
    """Delete specific cache"""
    bl_idname = "rizom.delete_cache"
    bl_label = "Delete Cache"
    
    cache_filename: bpy.props.StringProperty()
    
    def execute(self, context):
        if RizomCacheManager.delete_cache(self.cache_filename):
            self.report({'INFO'}, f"✓ Deleted: {self.cache_filename}")
        else:
            self.report({'WARNING'}, f"Could not delete")
        return {'FINISHED'}

class RIZOM_OT_clear_cache(bpy.types.Operator):
    """Clear all caches"""
    bl_idname = "rizom.clear_cache"
    bl_label = "Clear All Caches"
    
    def invoke(self, context, event):
        return context.window_manager.invoke_confirm(self, event)
    
    def execute(self, context):
        count = RizomCacheManager.clear_all_caches()
        self.report({'INFO'}, f"✓ Deleted {count} caches")
        return {'FINISHED'}

# ============================================================================
# UI PANEL
# ============================================================================

class VIEW3D_PT_rizom_bridge(bpy.types.Panel):
    """RizomUV Bridge UI"""
    bl_label = "RizomUV Bridge Pro"
    bl_idname = "VIEW3D_PT_rizom_bridge"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "RizomUV"
    
    def draw(self, context):
        layout = self.layout
        
        # Header
        box = layout.box()
        row = box.row()
        row.label(text="RizomUV Bridge Pro", icon='BLENDER')
        
        caches = RizomCacheManager.list_all_caches()
        row.label(text=f"Cache: {len(caches)}")
        
        # Import/Export
        layout.separator()
        col = layout.column(align=True)
        col.scale_y = 1.5
        col.operator("import_scene.fbx_rizom", 
                    text="1. Import FBX", icon='IMPORT')
        col.operator("export_scene.fbx_rizom_pro", 
                    text="2. Export + Inject", icon='EXPORT')
        
        # File menu info
        layout.separator()
        layout.label(text="Also in File > Import/Export", icon='INFO')
        
        # Clear cache
        layout.separator()
        layout.operator("rizom.clear_cache", text="Clear All Caches")
        
        # Cache list
        if len(caches) > 0:
            layout.separator()
            box = layout.box()
            box.label(text="Cached Files:", icon='FILE')
            
            for cache in caches:
                row = box.row()
                
                col_info = row.column()
                col_info.label(text=cache['fbx_name'])
                
                col_size = row.column()
                col_size.label(text=f"{cache['size_kb']:.0f} KB")
                
                col_del = row.column()
                op = col_del.operator("rizom.delete_cache", text="", icon='TRASH')
                op.cache_filename = cache['filename']

# ============================================================================
# MENU ENTRIES
# ============================================================================

def menu_import_func(self, context):
    self.layout.operator("import_scene.fbx_rizom", text="RizomUV Bridge (.fbx)")

def menu_export_func(self, context):
    self.layout.operator("export_scene.fbx_rizom_pro", text="RizomUV Bridge (.fbx)")

# ============================================================================
# REGISTRATION
# ============================================================================

classes = (
    IMPORT_OT_fbx_rizom,
    EXPORT_OT_fbx_rizom_pro,
    RIZOM_OT_delete_cache,
    RIZOM_OT_clear_cache,
    VIEW3D_PT_rizom_bridge,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    
    # Add to File menu
    bpy.types.TOPBAR_MT_file_import.append(menu_import_func)
    bpy.types.TOPBAR_MT_file_export.append(menu_export_func)
    
    print("[RizomUV Bridge PRO] v2.1.2 Registered ✓")

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    
    # Remove from File menu
    bpy.types.TOPBAR_MT_file_import.remove(menu_import_func)
    bpy.types.TOPBAR_MT_file_export.remove(menu_export_func)
    
    print("[RizomUV Bridge PRO] Unregistered")

if __name__ == "__main__":
    register()