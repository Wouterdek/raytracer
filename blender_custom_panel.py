import bpy

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
        self.new_prop(box, scene, "EnvironmentIntensity", "EnvironmentIntensity")

# REGISTRATION

def register():
    bpy.utils.register_class(SetupExtraProps)
    bpy.utils.register_class(ExtraPropsPanel)
    bpy.utils.register_class(SceneSetupExtraProps)
    bpy.utils.register_class(SceneExtraPropsPanel)


def unregister():
    bpy.utils.unregister_class(SetupExtraProps)
    bpy.utils.unregister_class(ExtraPropsPanel)
    bpy.utils.unregister_class(SceneSetupExtraProps)
    bpy.utils.unregister_class(SceneExtraPropsPanel)


if __name__ == "__main__":
    register()
