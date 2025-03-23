#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <iostream>
#include <vector>
#include <array>

#if defined(DEBUG) | defined(_DEBUG)
#define LogCond(y,x) if(y){Log(x)}
#if defined(_WIN64)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#define Log(x)\
{\
    std::stringstream s1768;  s1768 << x; \
    std::string str = s1768.str(); \
    std::wstring wstr(str.begin(), str.end()); \
    std::wstringstream wss(wstr); \
    OutputDebugString(wss.str().c_str()); \
}
#elif defined(__APPLE__)
#define Log(x)\
{\
    std::stringstream s1765; s1765 << x; \
	printf("%s", s1765.str().c_str()); \
}
#else
#define Log(x)\
{\
    std::stringstream s1765; s1765 << x; \
}
#endif
#else
#define Log(x)
#define LogCond(y,x)
#endif

// Function to load a texture using stb_image
unsigned char* LoadTextureData(const std::string& filePath, int& width, int& height, int& channels) {
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        Log("Failed to load texture: " << filePath << std::endl);
    }
    return data;
}

// Function to create a tinygltf texture from loaded texture data
tinygltf::Texture CreateTinyGltfTexture(tinygltf::Model& model, const std::string& filePath) {
    int width, height, channels;
    unsigned char* data = LoadTextureData(filePath, width, height, channels);
    if (!data) {
        return tinygltf::Texture();
    }

    tinygltf::Image image;
    image.width = width;
    image.height = height;
    image.component = channels;
    image.bits = 8;
    image.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
    image.image.resize(width * height * channels);
    std::copy(data, data + width * height * channels, image.image.begin());
    stbi_image_free(data);

    model.images.push_back(image);

    tinygltf::Texture texture;
    texture.source = static_cast<int>(model.images.size() - 1);
    model.textures.push_back(texture);

    return texture;
}

// Function to create a tinygltf material
tinygltf::Material CreateTinyGltfMaterial(tinygltf::Model& model, const std::vector<std::string>& mapFiles) {
    tinygltf::Material material;
    tinygltf::PbrMetallicRoughness pbr;

    for (const auto& filePath : mapFiles) {
        tinygltf::Texture texture = CreateTinyGltfTexture(model, filePath);
        if (filePath.find("Color") != std::string::npos) {
            pbr.baseColorTexture.index = static_cast<int>(model.textures.size() - 1);
        } else if (filePath.find("Normal") != std::string::npos) {
            material.normalTexture.index = static_cast<int>(model.textures.size() - 1);
        } else if (filePath.find("Roughness") != std::string::npos) {
            pbr.roughnessFactor = 1.0;
            pbr.metallicRoughnessTexture.index = static_cast<int>(model.textures.size() - 1);
        } else if (filePath.find("Metalness") != std::string::npos) {
            pbr.metallicFactor = 1.0;
            pbr.metallicRoughnessTexture.index = static_cast<int>(model.textures.size() - 1);
        } else if (filePath.find("AmbientOcclusion") != std::string::npos) {
            material.occlusionTexture.index = static_cast<int>(model.textures.size() - 1);
        }
    }

    material.pbrMetallicRoughness = pbr;
    model.materials.push_back(material);

    return material;
}

// Function to create a tinygltf mesh
tinygltf::Mesh CreateTinyGltfMesh(tinygltf::Model& model) {
    tinygltf::Mesh mesh;
    tinygltf::Primitive primitive;

    std::vector<float> positions = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    std::vector<uint16_t> indices = {0, 1, 2};

    tinygltf::Buffer buffer;
    buffer.data.resize(positions.size() * sizeof(float) + indices.size() * sizeof(uint16_t));
    std::memcpy(buffer.data.data(), positions.data(), positions.size() * sizeof(float));
    std::memcpy(buffer.data.data() + positions.size() * sizeof(float), indices.data(), indices.size() * sizeof(uint16_t));
    model.buffers.push_back(buffer);

    tinygltf::BufferView positionBufferView;
    positionBufferView.buffer = 0;
    positionBufferView.byteOffset = 0;
    positionBufferView.byteLength = positions.size() * sizeof(float);
    positionBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(positionBufferView);

    tinygltf::BufferView indexBufferView;
    indexBufferView.buffer = 0;
    indexBufferView.byteOffset = positions.size() * sizeof(float);
    indexBufferView.byteLength = indices.size() * sizeof(uint16_t);
    indexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    model.bufferViews.push_back(indexBufferView);

    tinygltf::Accessor positionAccessor;
    positionAccessor.bufferView = 0;
    positionAccessor.byteOffset = 0;
    positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    positionAccessor.count = static_cast<int>(positions.size() / 3);
    positionAccessor.type = TINYGLTF_TYPE_VEC3;
    model.accessors.push_back(positionAccessor);

    tinygltf::Accessor indexAccessor;
    indexAccessor.bufferView = 1;
    indexAccessor.byteOffset = 0;
    indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    indexAccessor.count = static_cast<int>(indices.size());
    indexAccessor.type = TINYGLTF_TYPE_SCALAR;
    model.accessors.push_back(indexAccessor);

    primitive.attributes["POSITION"] = 0;
    primitive.indices = 1;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    model.meshes.push_back(mesh);

    return mesh;
}

// Function to create a tinygltf scene
tinygltf::Scene CreateTinyGltfScene(tinygltf::Model& model) {
    tinygltf::Scene scene;
    tinygltf::Node node;

    node.mesh = 0;
    model.nodes.push_back(node);

    scene.nodes.push_back(0);
    model.scenes.push_back(scene);

    return scene;
}

bool ValidateTinyGltfModel(const tinygltf::Model& model) {
    tinygltf::TinyGLTF gltf;
    std::string err;
    std::string warn;

    // Serialize the model to a file
    if (!gltf.WriteGltfSceneToFile(&model, "temp_validation.glb", true, true, true, true)) {
        Log("Failed to serialize model to file" << std::endl);
        return false;
    }

    // Deserialize the model from the file
    tinygltf::Model tempModel;
    bool valid = gltf.LoadBinaryFromFile(&tempModel, &err, &warn, "temp_validation.glb");

    if (!valid) {
        if (!warn.empty()) {
            Log("Warnings: " << warn << std::endl);
        }
        if (!err.empty()) {
            Log("Errors: " << err << std::endl);
        }
    }
    return valid;
}

int main() {
    std::vector<std::string> mapFiles = {
        "C:/dev/cpp/data/raw/desert_Colormap_0_0.png",
        "C:/dev/cpp/data/raw/desert_Normal Map_0_0.png",
        "C:/dev/cpp/data/raw/desert_Roughness Map_0_0.png",
        "C:/dev/cpp/data/raw/desert_Metalness Map_0_0.png",
        "C:/dev/cpp/data/raw/desert_AmbientOcclusionMap_0_0.png"
    };

    tinygltf::Model model;
    tinygltf::Material material = CreateTinyGltfMaterial(model, mapFiles);
    tinygltf::Mesh mesh = CreateTinyGltfMesh(model);
    tinygltf::Scene scene = CreateTinyGltfScene(model);

    model.defaultScene = 0;

    if (!ValidateTinyGltfModel(model)) {
        Log("Model validation failed" << std::endl);
        return -1;
    }

    tinygltf::TinyGLTF gltf;
    if (!gltf.WriteGltfSceneToFile(&model, "output.glb", true, true, true, true)) {
        Log("Failed to write glTF file" << std::endl);
        return -1;
    }

    Log("Exported successfully to output.glb" << std::endl);

    return 0;
}
