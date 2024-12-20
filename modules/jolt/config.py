# config.py


def can_build(env, platform):
    env.module_add_dependencies("jolt", ["network_synchronizer"])
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "JBody3D",
        "JBoxShape3D",
        "JCapsuleShape3D",
        "JCompoundShape3D",
        "JConvexHullShape3D",
        "JCylinderShape3D",
        "JHeightFieldShape3D",
        "JLayersTable",
        "JMeshShape3D",
        "Jolt",
        "JoltDebugGeometry3D",
        "JShape3D",
        "JSphereShape3D",
    ]


def get_doc_path():
    return "doc_classes"
