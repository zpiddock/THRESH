//
// Created by shad0w on 20/05/2026.
//

#include "scene_serializer.hpp"

#include "glaze/glaze.hpp"

#include "ecs_types.hpp"
#include "substratum/filesystem/vfs.hpp"
#include "substratum/log.hpp"

namespace thresh {

        auto SceneSerializer::register_components(flecs::world &world) -> void {

          world.component<std::string>().opaque(flecs::String)
          .serialize([] (const flecs::serializer* s, const std::string* data) {
              const char* str = data->c_str();
              return s->value(flecs::String, str);
          })
          .assign_string([] (std::string* data, const char* value) {
              *data = value ? value : "";
          });

            register_math_components(world);

            world.component<Transform>()
            .member<flux::float3>("position")
            .member<flux::quat>("rotation")
            .member<flux::float3>("scale");

            world.component<Camera>()
            .member<float>("fov")
            .member<float>("near_plane")
            .member<float>("far_plane");

            world.component<CameraController>()
        .member<float>("yaw")
        .member<float>("pitch")
        .member<float>("movement_speed")
        .member<float>("mouse_sensitivity")
        .member<bool>("movement_allowed");

            world.component<Light>()
                .member<flux::float3>("colour")
                .member<float>("intensity");

            world.component<MeshSource>()
                .member<std::string>("source");

            world.component<MaterialSource>()
                .member<std::string>("path");

            // Mesh is runtime-derived. Registered so flecs to_json/from_json handles
            // it cleanly; values are overwritten on load by the resolution pass.
            world.component<Mesh>()
                .member<std::uint32_t>("handle")
                .member<std::uint32_t>("material_handle");

            // ActiveCamera is a pure tag — registering the component is enough.
            world.component<ActiveCamera>();

            world.component<SceneRoot>();

        }

        auto SceneSerializer::save_scene(Scene &scene, const std::string &vfs_path)
                                                                        -> bool {

            auto query = scene.get_world().query_builder()
                .with(flecs::ChildOf, scene.root())
                .cached()
                .build();

            auto iterator = query.iter();

            ecs_iter_to_json_desc_t desc{};
            desc.serialize_table = true;
            desc.serialize_values = true;
            desc.serialize_fields = true;
            desc.serialize_builtin = false;
            desc.serialize_entity_ids = false;
            desc.serialize_inherited = false;
            desc.serialize_full_paths = true;

            auto raw_out = iterator.to_json(&desc);
            if (!raw_out) {
                SUB_ERROR("ecs_iter_to_json returned null");
                return false;
            }

            std::string json{raw_out};

            auto pretty_json = glz::prettify_json(json);

            SUB_TRACE("{}", json);

            substratum::VFS::write_file_string("scene.json", pretty_json);

            return true;
        }

        auto SceneSerializer::load_scene(AssetsLoader &loader,
                                 const std::string &vfs_path)
                                  -> std::unique_ptr<Scene> {

            return nullptr;
        }

        auto SceneSerializer::register_math_components(flecs::world &world)
            -> void {

            world.component<flux::float2>()
            .member<float>("x")
            .member<float>("y");

            world.component<flux::float3>()
            .member<float>("x")
            .member<float>("y")
            .member<float>("z");

            world.component<flux::float4>()
            .member<float>("x")
            .member<float>("y")
            .member<float>("z")
            .member<float>("w");

            world.component<flux::quat>()
            .member<float>("x")
            .member<float>("y")
            .member<float>("z")
            .member<float>("w");
        }
} // thresh