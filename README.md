# The Mirror's fork of Godot Engine
![Bidirectionality Image](https://github.com/the-mirror-gdp/godot/assets/11920077/8d2ff842-b5bd-420b-8fcb-62a5e9efdc81)

The Mirror is a Roblox & UEFN alternative giving you the freedom to own what you create: an all-in-one game development platform.

**1. Join our [Discord](https://discord.com/invite/CK6fH3Cynk)**

**2. Check out our [Roadmap](https://github.com/orgs/the-mirror-gdp/projects)**

**3. Read our docs: [Site](https://docs.themirror.space), [monorepo source](https://github.com/the-mirror-gdp/the-mirror)**

**4. Visit our [website](https://themirror.space)**

**5. Follow us on [X/Twitter](https://twitter.com/themirrorgdp)**

This repo is a fork of Godot Engine, designed for The Mirror. The Mirror's fork includes several features submitted upstream as pull requests, and some features specific to The Mirror that are not suitable upstream.

Here is a highlight of some of the included features:

- `modules/app_protocol` adds support for protocols and IPC (inter-process communication) for deep linking, such as clicking on a link in a web browser to open your Godot application.
- `modules/godot_tracy` adds support for the Tracy profiler via AndreaCatania's [godot_tracy](https://github.com/AndreaCatania/godot_tracy) module.
- `modules/jolt` adds a custom Jolt implementation completely separate from Godot's built-in physics. This includes a general-purpose `JBody3D` node type that replaces all `CollisionObject3D`-derived node types. The GLTF module has been modified to support importing GLTF physics as these new nodes (on by default, can be disabled with one line of code change).
- `modules/network_synchronizer` adds support for network synchronization via AndreaCatania's [network_synchronizer](https://github.com/GameNetworking/network_synchronizer) module.
- `modules/the_mirror` adds many misc features designed for The Mirror, but can be used outside of The Mirror. Most classes provided by The Mirror have a `TM` prefix.
- The `TMUserGDScript` class adds support for dynamically loaded user-provided GDScript. This class has support for redirecting implicitly-self API calls to another object, allowing seamless support for multiple scripts per node or a component-based workflow. The GDScript module was modified to make this possible. Note: It does not handle sandboxing.
- The `TMAudioPlayer3D` node type supports switching between 3D and 0D, looping any audio stream, and accessing volume as a linear percentage.
- The `TMDataUtil` class provides utility functions for accessing data by JSON paths.
- The `TMFileUtil` class provides utility functions for dynamically loading files of various types.
- The `TMNodeUtil` class provides utility functions for working with descendant nodes.
- An upstream PR to improve PCK loading filename handling is included, allowing for multi-arch app distribution. [#59527](https://github.com/godotengine/godot/pull/59527)
- An upstream PR to allowing printing the stack trace is included. [#64205](https://github.com/godotengine/godot/pull/64205) Note: This PR is desired upstream, but needs to be improved first, and so has not been merged upstream yet.
- An upstream PR to add an annotation for abstract classes in GDScript is included. [#67777](https://github.com/godotengine/godot/pull/67777)
- An upstream PR to add a method to construct a NodePath from a StringName is included. [#72702](https://github.com/godotengine/godot/pull/72702)
- An upstream PR to change Node set_name to use StringName is included, which slightly improves its performance. [#76560](https://github.com/godotengine/godot/pull/76560)
- An upstream PR to allow accessing GraphEdit menu bar nodes by names is included. [#76563](https://github.com/godotengine/godot/pull/76563)
- An upstream PR to allow sorting Dictionaries is included, allowing for stable serialization, network transfer, and comparison. [#77213](https://github.com/godotengine/godot/pull/77213)
- An upstream PR to implement fit content width in TextEdit is included. [#83070](https://github.com/godotengine/godot/pull/83070)
- An upstream PR to add audio support to the GLTF module is included, allowing import and export of audio in glTF files. [#88204](https://github.com/godotengine/godot/pull/88204)
- An upstream PR to add support for explicitly-defined compound triggers in GLTF files is included. [#88301](https://github.com/godotengine/godot/pull/88301)
- A "Signaling Null" feature was added to Variant, allowing a single return value to contain an extra flag when null. This is used by `TMDataUtil`. Note: Signaling nulls are flattened to regular nulls when passed to GDScript, this is not intentional and we are not sure why it happens.
- All webcam-related classes, including `CameraFeed`, `CameraServer`, `CameraTexture`, and the `modules/camera` folder, are disabled because they were causing crashes in The Mirror.

## Community and contributing

If you believe your contribution would benefit all Godot users, prefer to
submit it to the upstream first. This is usually in the form of an issue,
proposal, or pull request. You can chat with Godot users on various community
channels listed [on the Godot homepage](https://godotengine.org/community).
The best way to get in touch with the core engine developers is to join the
[Godot Contributors Chat](https://chat.godotengine.org).
For more information about contributing to upstream Godot,
see the [Godot contributing guide](CONTRIBUTING.md).
This document also includes guidelines for reporting bugs.

If you believe your contribution is specific to The Mirror, or is otherwise
more niche, you can submit it here as an issue or pull request.
Ideally, any contribution submitted to The Mirror's fork of Godot should
either be useful to The Mirror or other games similar to The Mirror,
meaning real-time 3D games with dynamic content. For example, enhancements
to the [glTF](https://www.khronos.org/gltf/) importer.

Any pull request in this repo should be directed to the `themirror` branch.
However, note that this branch is subject to being force-pushed at any time,
so that the Git history stays clean, minimal, and easy-to-follow. We will
try to review all open pull requests before performing any force-pushing.
After merge, the exact commit hash of your PR will not be preserved.

[Discord](https://discord.com/invite/CK6fH3Cynk)

[X/Twitter](https://twitter.com/themirrorgdp)

[Roadmap](https://github.com/orgs/the-mirror-gdp/projects)

## What is Godot Engine?

**[Godot Engine](https://godotengine.org) is a feature-packed, cross-platform
game engine to create 2D and 3D games from a unified interface.** It provides a
comprehensive set of [common tools](https://godotengine.org/features), so that
users can focus on making games without having to reinvent the wheel. Games can
be exported with one click to a number of platforms, including the major desktop
platforms (Linux, macOS, Windows), mobile platforms (Android, iOS), as well as
Web-based platforms and [consoles](https://docs.godotengine.org/en/latest/tutorials/platform/consoles.html).

<p align="center">
  <a href="https://godotengine.org">
    <img src="logo_outlined.svg" width="400" alt="Godot Engine logo">
  </a>
</p>

### Free and open source

Godot is completely free and open source under the very permissive [MIT license](https://godotengine.org/license).
No strings attached, no royalties, nothing. The users' games are theirs, down
to the last line of engine code. It is supported by the
[Godot Foundation](https://godot.foundation/) non-profit.

Before being open sourced in [February 2014](https://github.com/godotengine/godot/commit/0b806ee0fc9097fa7bda7ac0109191c9c5e0a1ac),
Godot had been developed by [Juan Linietsky](https://github.com/reduz) and
[Ariel Manzur](https://github.com/punto-) (both still maintaining the project)
for several years as an in-house engine, used to publish several work-for-hire
titles.

## Getting the engine

### Compiling from source

The instructions for building The Mirror's fork of Godot are nearly the same
as for building stock Godot, as long as you are on the correct branch and
checkout the required submodules.

The `themirror` branch is the one with The Mirror's changes on it.
The `master` branch is the base branch that `themirror` is based on.

To checkout the Git submodules, you can use the following command:

```
git submodule update --init --recursive
```

After that, [see the official docs](https://docs.godotengine.org/en/latest/contributing/development/compiling)
for compilation instructions for every supported platform.

## Documentation

The Mirror's docs are in its monorepo: https://github.com/the-mirror-gdp/the-mirror and deployed to its docs site, https://docs.themirror.space. Feel free to contribute there via pull request.

The Mirror's engine-level features are documented in the class reference.
There may be some gaps in the documentation, as we are still working on it.

Godot's official documentation is hosted on
[Read the Docs](https://docs.godotengine.org).
It is maintained by the Godot community in its own
[GitHub repository](https://github.com/godotengine/godot-docs).

The [Godot class reference](https://docs.godotengine.org/en/latest/classes/)
is also accessible from the Godot editor.
