def can_build(env, platform):
    env.module_add_dependencies("gltf", ["jolt"])
    return not env["disable_3d"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "EditorSceneFormatImporterBlend",
        "EditorSceneFormatImporterGLTF",
        "GLTFAccessor",
        "GLTFAnimation",
        "GLTFAudioPlayer",
        "GLTFBufferView",
        "GLTFCamera",
        "GLTFDocument",
        "GLTFDocumentExtension",
        "GLTFDocumentExtensionConvertImporterMesh",
        "GLTFLight",
        "GLTFMesh",
        "GLTFNode",
        "GLTFPhysicsBody",
        "GLTFPhysicsShape",
        "GLTFSkeleton",
        "GLTFSkin",
        "GLTFSpecGloss",
        "GLTFState",
        "GLTFTexture",
        "GLTFTextureSampler",
    ]


def get_doc_path():
    return "doc_classes"
