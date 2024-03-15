def can_build(env, platform):
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "TMAudioPlayer3D",
        "TMDataUtil",
        "TMFileUtil",
        "TMNodeUtil",
        "TMShaderLanguage",
        "TMUserGDScript",
        "TMUserGDScriptSyntaxHighlighter",
    ]


def get_doc_path():
    return "doc_classes"
