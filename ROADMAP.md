# THRESH — Roadmap

Rough plan of what's next. Order inside each bucket is loose; sub-notes are reminders to me, not specs.

## Now

**Clean up what scene serialisation writes.** Right now save/load is round-tripping a bunch of stuff that should be runtime-only (GPU handles, derived state, etc.) — the on-disk file gets polluted with data the next run will recompute anyway. Two options to pick from:

  - Stop exposing `save_scene` from the runtime altogether and only call it from the (future) editor — runtime never writes scenes, period.
  - Add a `SaveState` (or similar) tag component to opt entities/components *in* to serialisation; the serializer's `(ChildOf, SceneRoot)` query filters by that tag.

Leaning towards the tag approach since it survives once an editor exists too, but worth thinking about. Either way the bar is: nothing runtime-only ends up on disk.

**Transform hierarchy — local vs world.** ✅ Done. `WorldTransform` is propagated from `Transform` via flecs `ChildOf` in a PostUpdate pass, with `WorldAABB` per-entity (plus aggregated up the tree) and a ray-vs-AABB `Scene::pick_entity` for ImGui picking. Outstanding: entities parented to the camera aren't fully handled — child entities get carried along by the propagation, but anything that wants to live in camera-local space (HUD billboards, first-person view-model) still needs separate view-space rendering work. Revisit alongside that.

**ImGui debug rendering.** ImGui is wired up already, just no debug-draw primitives. Want: lines, AABBs, and a basic scene inspector panel (entity tree + component fields) on top of `DearImGuiContext`. A buffered line-list flushed in its own tiny pipeline is probably enough for v1.

## Next

**Arbitrary pipeline API.** `VulkanContext::register_pipeline` exists but only gets called from inside flux with hardcoded `.spv` paths (opaque_mesh, composite). Want thresh to register pipelines from data — shader paths, vertex layout, descriptor/push-constant layout — without flux growing any file I/O (rule still stands: flux gets resolved POD only). Open question: do I want a `default_bindings.slang` include carrying the common scene/camera/light set, so user shaders don't have to redeclare it? Probably yes. Also fold in a `VkPipelineCache` here — single cache owned by `VulkanContext`, loaded from disk at startup and written back on shutdown, passed into every `vkCreateGraphicsPipelines` call. Cheap to add while the pipeline-creation site is being touched anyway, and the win compounds as we start dispatching shader variants by `material_type`.

**Basic lighting.** `Light { colour, intensity }` already exists and serializes, it's just orphaned — no shader reads it. One directional + an ambient term in a per-frame uniform, wire `opaque_mesh.slang` to actually use it. Direction comes from the entity's world Transform, so this lands after the hierarchy task.

**PBR materials.** Schema already carries albedo/normal/metallic/roughness/emission + tint + `material_type`, but the runtime shader only samples albedo. Author a real PBR forward shader (Cook-Torrance is fine), feed it the lighting uniforms, dispatch the shader variant via `material_type` through the pipeline API above. Keep Material surface-only — no mesh fields creeping in (this has bitten me before).

**glTF loading.** Right now meshes are just `flux::primitives::` (box, sphere). Add a `MeshAsset` to `thresh-asset-schema` (positions / normals / tangents / uvs / indices + per-submesh material refs), pick a parser — leaning fastgltf — and wire `AssetLoader` to produce MeshAssets that flux can register. Glb files with nested nodes won't work right until the transform hierarchy task is done, so don't merge these in the wrong order.

## Later

**Jolt Physics.** Nothing exists yet — no Jolt in CMake, no RigidBody/Collider components, no step. Vendor Jolt, add the components, step physics before transform propagation each frame, write results back to Transform. Open: how are colliders authored? Fields on the component vs a separate asset. Lean towards components for primitive shapes and an asset for meshes.

**Reverse-Z depth buffer.** Picking exposed the precision ceiling — `inverse(projection * view)` gets ill-conditioned as `far/near` grows, and the regular `[0,1]` depth distribution wastes most of its precision near the camera. Switch to reverse-Z (1.0 at near, 0.0 at far) with a float depth format and an infinite-far projection. Touchpoints: depth clear value, depth compare op (`GREATER_OR_EQUAL`), projection matrix builder in `Camera`/`compute_active_camera_data`, and the `[1][1] *= -1` Y-flip path. Picking math is unaffected — ray direction is invariant under the choice of far plane.

**Wren scripting (low priority).** No scripting hooks at all today. Eventually want Wren wired in with a small binding surface (entity/component get-set, input, time) and scenes referencing script assets. Deliberately deferred — if the renderer and physics layouts are still moving, the bindings will get reworked twice. Revisit once the rest of this list is mostly done.
