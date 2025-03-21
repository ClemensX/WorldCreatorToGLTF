#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <tiny_gltf.h>
#include <iostream>
#include <vector>

// Function to load a texture map using Assimp
aiTexture* LoadTexture(const std::string& filePath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || !scene->HasTextures()) {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        return nullptr;
    }
    return scene->mTextures[0];
}

// Function to create a glTF texture
tinygltf::Texture CreateGLTFTexture(tinygltf::Model& model, const aiTexture* aiTex) {
    tinygltf::Texture texture;
    tinygltf::Image image;
    tinygltf::Sampler sampler;

    // Fill image data
    image.width = aiTex->mWidth;
    image.height = aiTex->mHeight;
    image.component = 4; // Assuming RGBA
    image.image.resize(aiTex->mWidth * aiTex->mHeight * 4);
    memcpy(image.image.data(), aiTex->pcData, image.image.size());

    // Add image to model
    model.images.push_back(image);
    texture.source = static_cast<int>(model.images.size() - 1);

    // Add sampler to model
    model.samplers.push_back(sampler);
    texture.sampler = static_cast<int>(model.samplers.size() - 1);

    // Add texture to model
    model.textures.push_back(texture);
    return texture;
}

// Function to export maps to glTF 2.0
void ExportToGLTF(const std::string& outputFilePath, const std::vector<std::string>& mapFiles) {
    tinygltf::Model model;
    tinygltf::Material material;

    for (const auto& filePath : mapFiles) {
        aiTexture* aiTex = LoadTexture(filePath);
        if (aiTex) {
            tinygltf::Texture texture = CreateGLTFTexture(model, aiTex);
            // Assign textures to material based on file type
            if (filePath.find("Color") != std::string::npos) {
                material.pbrMetallicRoughness.baseColorTexture.index = texture.source;
            }
            else if (filePath.find("Normal") != std::string::npos) {
                material.normalTexture.index = texture.source;
            }
            else if (filePath.find("Roughness") != std::string::npos) {
                material.pbrMetallicRoughness.roughnessTexture.index = texture.source;
            }
            else if (filePath.find("Metalness") != std::string::npos) {
                material.pbrMetallicRoughness.metallicTexture.index = texture.source;
            }
            else if (filePath.find("AmbientOcclusion") != std::string::npos) {
                material.occlusionTexture.index = texture.source;
            }
        }
    }

    model.materials.push_back(material);

    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(&model, outputFilePath, true, true, true, true);
}

int main() {
    std::vector<std::string> mapFiles = {
        "path/to/ColorMap.png",
        "path/to/NormalMap.png",
        "path/to/RoughnessMap.png",
        "path/to/MetalnessMap.png",
        "path/to/AmbientOcclusionMap.png"
    };

    ExportToGLTF("output.gltf", mapFiles);

    return 0;
}
