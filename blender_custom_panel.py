import bpy
from bpy.props import StringProperty, IntProperty, CollectionProperty
from bpy.types import PropertyGroup, UIList, Operator, Panel

# OBJECT

class SetupExtraProps(bpy.types.Operator) :
    bl_idname = "obj.setup_extra_props"
    bl_label = "Setup Extra Props"
    bl_options = {"UNDO"}
    
    def new_prop(self, obj, prop_name, default_val, min = 0.0, max = 1.0, soft_min = 0.0, soft_max = 1.0):
        if prop_name not in obj:
            obj[prop_name] = default_val
            if "_RNA_UI" not in obj:
                obj["_RNA_UI"] = {}
            obj["_RNA_UI"][prop_name] = {"min":min, "max": max, "soft_min":soft_min, "soft_max":soft_max}

    def new_color_prop(self, obj, prop_name, default_val):
        if prop_name not in obj:
            obj[prop_name] = default_val
            if "_RNA_UI" not in obj:
                obj["_RNA_UI"] = {}
            obj["_RNA_UI"][prop_name] = {"subtype": "COLOR"}

    def invoke(self, context, event):
        obj = context.active_object
        if obj.type == "CAMERA":
            self.new_prop(obj, "IsMainCamera", False)
            self.new_prop(obj, "FStop", 0.0, max = 1000.0, soft_max = 1000.0)
            self.new_prop(obj, "FocalDistance", 0.0, max = 1000.0, soft_max = 1000.0)
        else:
            if obj.type == "EMPTY" or obj.type == "LIGHT":
                self.new_prop(obj, "IsPointLight", False)
                self.new_prop(obj, "IsDirectionalLight", False)
                self.new_prop(obj, "DirectionalLight.Angle", 0.0)
            elif obj.type == "MESH":
                self.new_prop(obj, "IsAreaLight", False)
            self.new_prop(obj, "LightIntensity", 500.0, max = 1000.0, soft_max = 1000.0)
            self.new_color_prop(obj, "LightColor", [1.0, 1.0, 1.0])
            
            if obj.type == "MESH":
                self.new_prop(obj, "Material.Transmission", 0.0)
                self.new_prop(obj, "Material.IOR", 1.45, max = 10.0, soft_max = 10.0)
                self.new_prop(obj, "Material.AttenuationStrength", 5.00, max = 1000.0, soft_max = 100.0)
                self.new_prop(obj, "Material.ClearCoat", 0.0)
                self.new_prop(obj, "Material.ClearCoatIOR", 1.45, max = 10.0, soft_max = 10.0)
                self.new_prop(obj, "Material.Alpha", 1.0, max = 1.0, soft_max = 1.0)
                
        return {"FINISHED"}

class ExtraPropsPanel(bpy.types.Panel):
    bl_label = "Raytracer Properties"
    bl_idname = "OBJECT_PT_extra_props"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"
    
    def new_prop(self, layout, obj, prop_name, label):
        row = layout.row()
        row.label(text=label)
        row.prop(obj, '["'+prop_name+'"]', text="")

    def draw(self, context):
        layout = self.layout

        obj = context.object
        
        row = layout.row()
        row.operator("obj.setup_extra_props", text = "Setup properties")
        
        if obj.type == "CAMERA":
            box = layout.box()
            row = box.row()
            row.label(text="Camera", icon='CAMERA_DATA')
            self.new_prop(box, obj, "IsMainCamera", "IsMainCamera")
            self.new_prop(box, obj, "FStop", "F-Stop")
            self.new_prop(box, obj, "FocalDistance", "FocalDistance")
        else:
            box = layout.box()
            row = box.row()
            row.label(text="Light", icon='LIGHT')
            if obj.type == "EMPTY" or obj.type == "LIGHT":
                self.new_prop(box, obj, "IsPointLight", "IsPointLight")
                self.new_prop(box, obj, "IsDirectionalLight", "IsDirectionalLight")
                self.new_prop(box, obj, "DirectionalLight.Angle", "DirectionalLight.Angle")
            elif obj.type == "MESH":
                self.new_prop(box, obj, "IsAreaLight", "IsAreaLight")
            self.new_prop(box, obj, "LightIntensity", "LightIntensity")
            self.new_prop(box, obj, "LightColor", "LightColor")
            
            if obj.type == "MESH":
                box = layout.box()
                row = box.row()
                row.label(text="Material", icon='MATERIAL')
                self.new_prop(box, obj, "Material.Transmission", "Transmission")
                self.new_prop(box, obj, "Material.IOR", "IOR")
                self.new_prop(box, obj, "Material.AttenuationStrength", "AttenuationStrength")
                self.new_prop(box, obj, "Material.ClearCoat", "ClearCoat")
                self.new_prop(box, obj, "Material.ClearCoatIOR", "ClearCoat IOR")
                self.new_prop(box, obj, "Material.Alpha", "Alpha")

# SCENE

class SceneSetupExtraProps(bpy.types.Operator) :
    bl_idname = "scene.setup_extra_props"
    bl_label = "Setup Extra Props"
    bl_options = {"UNDO"}
    
    def new_prop(self, scene, prop_name, default_val, min = 0.0, max = 1.0, soft_min = 0.0, soft_max = 1.0):
        if prop_name not in scene:
            scene[prop_name] = default_val
            if "_RNA_UI" not in scene:
                scene["_RNA_UI"] = {}
            scene["_RNA_UI"][prop_name] = {"min":min, "max": max, "soft_min":soft_min, "soft_max":soft_max}

    def new_color_prop(self, scene, prop_name, default_val):
        if prop_name not in scene:
            scene[prop_name] = default_val
            if "_RNA_UI" not in scene:
                obj["_RNA_UI"] = {}
            scene["_RNA_UI"][prop_name] = {"subtype": "COLOR"}

    def invoke(self, context, event):
        scene = context.scene
        self.new_prop(scene, "EnvironmentIntensity", 000.0, max = 1000.0, soft_max = 1000.0)
        self.new_prop(scene, "EnvironmentHDRIFilePath", "")
        self.new_color_prop(scene, "EnvironmentColor", [1.0, 1.0, 1.0])
                
        return {"FINISHED"}

class SceneExtraPropsPanel(bpy.types.Panel):
    bl_label = "Raytracer Properties"
    #bl_idname = "OBJECT_PT_extra_props"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"
    
    def new_prop(self, layout, scene, prop_name, label):
        row = layout.row()
        row.label(text=label)
        row.prop(scene, '["'+prop_name+'"]', text="")

    def draw(self, context):
        layout = self.layout

        scene = context.scene
        
        row = layout.row()
        row.operator("scene.setup_extra_props", text = "Setup properties")
        
        box = layout.box()
        row = box.row()
        row.label(text="Scene", icon='SCENE')
        self.new_prop(box, scene, "EnvironmentColor", "EnvironmentColor")
        self.new_prop(box, scene, "EnvironmentHDRIFilePath", "EnvironmentHDRIFilePath")
        self.new_prop(box, scene, "EnvironmentIntensity", "EnvironmentIntensity")

# MATERIAL

class MaterialSetupExtraProps(bpy.types.Operator) :
    bl_idname = "material.setup_extra_props"
    bl_label = "Setup Extra Props"
    bl_options = {"UNDO"}
    
    def new_prop(self, scene, prop_name, default_val, min = 0.0, max = 1.0, soft_min = 0.0, soft_max = 1.0):
        if prop_name not in scene:
            scene[prop_name] = default_val
            if "_RNA_UI" not in scene:
                scene["_RNA_UI"] = {}
            scene["_RNA_UI"][prop_name] = {"min":min, "max": max, "soft_min":soft_min, "soft_max":soft_max}

    def new_color_prop(self, scene, prop_name, default_val):
        if prop_name not in scene:
            scene[prop_name] = default_val
            if "_RNA_UI" not in scene:
                obj["_RNA_UI"] = {}
            scene["_RNA_UI"][prop_name] = {"subtype": "COLOR"}

    def invoke(self, context, event):
        material = context.active_object.active_material
                
        return {"FINISHED"}

class MaterialAddVideoFrameDefinition(bpy.types.Operator) :
    bl_idname = "material.add_video_frame_definition"
    bl_label = "New"
    bl_options = {"UNDO"}

    def invoke(self, context, event):
        material = context.active_object.active_material
        if "VideoImageMapping" not in material:
            material["VideoImageMapping"] = []

        newArr = []
        for i in range(len(material["VideoImageMapping"])):
            newArr.append(material["VideoImageMapping"][i])
        newArr.append({"ImageName": "a", "FrameDirectory": "P", "Frame": 1})
        material["VideoImageMapping"] = newArr


        return {"FINISHED"}

class MaterialRemoveVideoFrameDefinition(bpy.types.Operator) :
    bl_idname = "material.remove_video_frame_definition"
    bl_label = "Delete"
    bl_options = {"UNDO"}

    def invoke(self, context, event):
        material = context.active_object.active_material
        if "VideoImageMapping" not in material:
            pass

        newArr = []
        for i in range(len(material["VideoImageMapping"])):
            if i == material.active_vim_index:
                continue
            newArr.append(material["VideoImageMapping"][i])
        material["VideoImageMapping"] = newArr


        return {"FINISHED"}
class MATERIAL_UL_VideoFrameDefinition(bpy.types.UIList):
    # The draw_item function is called for each item of the collection that is visible in the list.
    #   data is the RNA object containing the collection,
    #   item is the current drawn item of the collection,
    #   icon is the "computed" icon for the item (as an integer, because some objects like materials or textures
    #   have custom icons ID, which are not available as enum items).
    #   active_data is the RNA object containing the active property for the collection (i.e. integer pointing to the
    #   active item of the collection).
    #   active_propname is the name of the active property (use 'getattr(active_data, active_propname)').
    #   index is index of the current item in the collection.
    #   flt_flag is the result of the filtering process for this item.
    #   Note: as index and flt_flag are optional arguments, you do not have to use/declare them here if you don't
    #         need them.
    def draw_item(self, context, layout, material, entry, icon, active_material, active_propname):
        # draw_item must handle the three layout types... Usually 'DEFAULT' and 'COMPACT' can share the same code.
        if self.layout_type in {'DEFAULT', 'COMPACT'}:
            # You should always start your row layout by a label (icon + text), or a non-embossed text field,
            # this will also make the row easily selectable in the list! The later also enables ctrl-click rename.
            # We use icon_value of label, as our given icon is an integer value, not an enum ID.
            # Note "data" names should never be translated!
            layout.prop(entry, '["ImageName"]', text="")
            layout.prop(entry, '["FrameDirectory"]', text="")
            layout.prop(entry, '["Frame"]', text="")
        # 'GRID' layout type should be as compact as possible (typically a single icon!).
        elif self.layout_type in {'GRID'}:
            layout.alignment = 'CENTER'
            layout.label(text="", icon_value=icon)

class MaterialExtraPropsPanel(bpy.types.Panel):
    bl_label = "Raytracer Properties"
    #bl_idname = "OBJECT_PT_extra_props"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "material"

    def __init__(self):
        self.active_vim_index = 0
    
    def new_prop(self, layout, material, prop_name, label):
        row = layout.row()
        row.label(text=label)
        row.prop(material, '["'+prop_name+'"]', text="")

    def draw(self, context):
        layout = self.layout

        material = context.active_object.active_material
        
        row = layout.row()
        row.operator("material.setup_extra_props", text = "Setup properties")
        
        box = layout.box()
        row = box.row()
        row.label(text="Material", icon='MATERIAL')

        row = box.row()
        row.label(text="Image Video Mapping")
        row.template_list("MATERIAL_UL_VideoFrameDefinition", "", material, '["VideoImageMapping"]', material, 'active_vim_index', rows=1)
        row = box.row()
        row.operator("material.add_video_frame_definition", text = "+")
        row.operator("material.remove_video_frame_definition", text = "-")

# REGISTRATION

def register():
    bpy.utils.register_class(SetupExtraProps)
    bpy.utils.register_class(ExtraPropsPanel)
    bpy.utils.register_class(SceneSetupExtraProps)
    bpy.utils.register_class(SceneExtraPropsPanel)
    bpy.utils.register_class(MaterialSetupExtraProps)
    bpy.utils.register_class(MaterialAddVideoFrameDefinition)
    bpy.utils.register_class(MaterialRemoveVideoFrameDefinition)
    bpy.utils.register_class(MATERIAL_UL_VideoFrameDefinition)
    bpy.types.Material.active_vim_index = IntProperty(name = "Index for VIM", default = 0)
    bpy.utils.register_class(MaterialExtraPropsPanel)

def unregister():
    bpy.utils.unregister_class(SetupExtraProps)
    bpy.utils.unregister_class(ExtraPropsPanel)
    bpy.utils.unregister_class(SceneSetupExtraProps)
    bpy.utils.unregister_class(SceneExtraPropsPanel)
    bpy.utils.unregister_class(MaterialSetupExtraProps)
    bpy.utils.unregister_class(MaterialAddVideoFrameDefinition)
    bpy.utils.unregister_class(MaterialRemoveVideoFrameDefinition)
    bpy.utils.unregister_class(MATERIAL_UL_VideoFrameDefinition)
    del bpy.types.Material.active_vim_index
    bpy.utils.unregister_class(MaterialExtraPropsPanel)


if __name__ == "__main__":
    register()
