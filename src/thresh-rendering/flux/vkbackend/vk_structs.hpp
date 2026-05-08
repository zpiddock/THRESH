//
// Created by Admin on 06/05/2026.
//

#pragma once
#include <string>
#include <vector>

namespace flux {

    struct VulkanInstanceContext {
        std::string              application_name;
        std::string              engine_name               = "THRΞSH";
        std::string              engine_version            = "0.0.1";
        std::string              application_version       = "0.0.1";
        bool                     enable_validation_layers  = true;
        std::vector<const char*> enabled_validation_layers = {
            "VK_LAYER_KHRONOS_validation"
        };
        std::vector<const char*> required_instance_extensions = {};
    };
}
