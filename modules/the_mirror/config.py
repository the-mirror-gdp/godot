def can_build(env, platform):
    env.module_add_dependencies("the_mirror", ["network_synchronizer", "jolt"])
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "CollisionFxManager",
        "TMAudioPlayer3D",
        "TMCharacter3D",
        "TMDataUtil",
        "TMFileUtil",
        "TMNodeUtil",
        "TMSceneSync",
        "TMShaderLanguage",
        "TMSpaceObjectBase",
        "TMUserGDScript",
        "TMUserGDScriptSyntaxHighlighter",
    ]


def get_doc_path():
    return "doc_classes"
