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

    // Create a tinygltf::Image
    tinygltf::Image image{};
    image.width = width;
    image.height = height;
    image.component = channels;
    image.bits = 8;
    image.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
    image.image.resize(width * height * channels);
    std::copy(data, data + width * height * channels, image.image.begin());
    stbi_image_free(data);

    // Create a tinygltf::Buffer and tinygltf::BufferView for the image
    //tinygltf::Buffer buffer{};
    //buffer.data.resize(width * height * channels);
    //std::copy(data, data + width * height * channels, buffer.data.begin());
    //stbi_image_free(data);

    //tinygltf::BufferView bufferView{};
    //bufferView.buffer = static_cast<int>(model.buffers.size());
    //bufferView.byteOffset = 0;
    //bufferView.byteLength = static_cast<int>(buffer.data.size());
    //bufferView.target = 0; // No specific target

    //// Add the buffer and buffer view to the model
    //model.buffers.push_back(buffer);
    //model.bufferViews.push_back(bufferView);

    //// Set the buffer view for the image
    //image.bufferView = static_cast<int>(model.bufferViews.size() - 1);
    //image.mimeType = "image/png"; // Set the appropriate MIME type
    image.uri = filePath;

    // Add the image to the model
    model.images.push_back(image);

    // Create a tinygltf::Texture
    tinygltf::Texture texture{};
    texture.source = static_cast<int>(model.images.size() - 1);
    model.textures.push_back(texture);

    return texture;
}

// Function to add textures to tinygltf material
void AddTexturesToMaterial(tinygltf::Model& model, const std::vector<std::string>& mapFiles) {
    assert(model.materials.size() == 1);
    tinygltf::Material &material = model.materials[0];
    tinygltf::PbrMetallicRoughness &pbr = material.pbrMetallicRoughness;

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

void createModel(tinygltf::Model& m) {
    // Create a model with a single mesh and save it as a gltf file
    tinygltf::Scene scene;
    tinygltf::Mesh mesh;
    tinygltf::Primitive primitive;
    tinygltf::Node node;
    tinygltf::Asset asset;

    // define buffer data: positions, inices and text coords
    std::vector<float> positions = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    std::vector<uint16_t> indices = { 0, 1, 2, 0, 1, 2 }; // doubled length for 4 byte multiple
    //std::vector<float> positions = {
    //    // 36 bytes of floating point numbers
    //    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //    0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x3f,
    //    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //    0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x3f,
    //    0x00,0x00,0x00,0x00 };
    //// 6 bytes of indices and two bytes of padding
    //std::vector<uint16_t> indices = { 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00 };
    std::vector<float> texcoords = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f };

    // create buffer from the input data
    tinygltf::Buffer buffer;
    size_t totalBufferLength = positions.size() * sizeof(float) + indices.size() * sizeof(uint16_t) + texcoords.size() * sizeof(float);
    buffer.data.resize(totalBufferLength);
    std::memcpy(buffer.data.data(), positions.data(), positions.size() * sizeof(float));
    std::memcpy(buffer.data.data() + positions.size() * sizeof(float), indices.data(), indices.size() * sizeof(uint16_t));
    std::memcpy(buffer.data.data() + positions.size() * sizeof(float) + indices.size() * sizeof(uint16_t), texcoords.data(), texcoords.size() * sizeof(float));
    m.buffers.push_back(buffer);

    // define the buffer views
    tinygltf::BufferView positionBufferView;
    positionBufferView.buffer = 0;
    positionBufferView.byteOffset = 0;
    positionBufferView.byteLength = positions.size() * sizeof(float);
    positionBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    m.bufferViews.push_back(positionBufferView);

    tinygltf::BufferView indexBufferView;
    indexBufferView.buffer = 0;
    indexBufferView.byteOffset = positions.size() * sizeof(float);
    indexBufferView.byteLength = indices.size() * sizeof(uint16_t);
    indexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    m.bufferViews.push_back(indexBufferView);

    tinygltf::BufferView texcoordBufferView;
    texcoordBufferView.buffer = 0;
    texcoordBufferView.byteOffset = positions.size() * sizeof(float) + indices.size() * sizeof(uint16_t);
    texcoordBufferView.byteLength = texcoords.size() * sizeof(float);
    texcoordBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    m.bufferViews.push_back(texcoordBufferView);

    // define the accessors

    tinygltf::Accessor positionAccessor;
    positionAccessor.bufferView = 0;
    positionAccessor.byteOffset = 0;
    positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    positionAccessor.count = static_cast<int>(positions.size() / 3);
    positionAccessor.type = TINYGLTF_TYPE_VEC3;
    positionAccessor.maxValues = { 1.0, 1.0, 0.0 };
    positionAccessor.minValues = { 0.0, 0.0, 0.0 };
    m.accessors.push_back(positionAccessor);

    tinygltf::Accessor indexAccessor;
    indexAccessor.bufferView = 1;
    indexAccessor.byteOffset = 0;
    indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    indexAccessor.count = static_cast<int>(indices.size());
    indexAccessor.count = static_cast<int>(3); // override
    indexAccessor.type = TINYGLTF_TYPE_SCALAR;
    m.accessors.push_back(indexAccessor);

    tinygltf::Accessor texcoordAccessor;
    texcoordAccessor.bufferView = 2;
    texcoordAccessor.byteOffset = 0;
    texcoordAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    texcoordAccessor.count = static_cast<int>(texcoords.size() / 2);
    texcoordAccessor.type = TINYGLTF_TYPE_VEC2;
    m.accessors.push_back(texcoordAccessor);

    // Build the mesh primitive and add it to the mesh
    primitive.attributes["POSITION"] = 0;
    primitive.indices = 1;
    primitive.attributes["TEXCOORD_0"] = 2;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    primitive.material = 0;
    mesh.primitives.push_back(primitive);

    // Other tie ups
    node.mesh = 0;
    scene.nodes.push_back(0); // Default scene

    // Define the asset. The version is required
    asset.version = "2.0";
    asset.generator = "tinygltf";

    // Now all that remains is to tie back all the loose objects into the
    // our single model.
    m.scenes.push_back(scene);
    m.meshes.push_back(mesh);
    m.nodes.push_back(node);
    m.asset = asset;

    // Create a simple material
    tinygltf::Material mat;
    mat.pbrMetallicRoughness.baseColorFactor = { 1.0f, 0.9f, 0.9f, 1.0f };
    mat.doubleSided = true;
    m.materials.push_back(mat);
}

int main() {
    tinygltf::Model model;
    createModel(model);
    std::vector<std::string> mapFiles = {
        //"Cube_BaseColor.png"//,
        "C:/dev/cpp/data/raw/desert_Colormap_0_0.png",
        "C:/dev/cpp/data/raw/desert_Normal Map_0_0.png"
        "C:/dev/cpp/data/raw/desert_Roughness Map_0_0.png",
        "C:/dev/cpp/data/raw/desert_Metalness Map_0_0.png",
        "C:/dev/cpp/data/raw/desert_AmbientOcclusionMap_0_0.png"
    };

    AddTexturesToMaterial(model, mapFiles);

    //if (!ValidateTinyGltfModel(model)) {
    //    Log("Model validation failed" << std::endl);
    //    return -1;
    //}

    // Save it to a file
    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(&model, "triangle.gltf",
        true, // embedImages
        true, // embedBuffers
        true, // pretty print
        false); // write binary

    // writing glb complains with warninig:: URI is used in GLB container.
    //if (!gltf.WriteGltfSceneToFile(&model, "output.glb", true, true, true, true)) {
    //    Log("Failed to write glTF file" << std::endl);
    //    return -1;
    //}

    Log("Exported successfully" << std::endl);

    return 0;
}

int main2(int argc, char* argv[]) {
    //if (argc != 3) {
    //    std::cout << "Needs input.gltf output.gltf" << std::endl;
    //    return EXIT_FAILURE;
    //}

    tinygltf::Model modelOK;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    std::string input_filename("Cube.gltf");
    std::string output_filename("Cube2.gltf");
    std::string embedded_filename =
        output_filename.substr(0, output_filename.size() - 5) + "-Embedded.gltf";

    // assume ascii glTF.
    bool ret = loader.LoadASCIIFromFile(&modelOK, &err, &warn, input_filename.c_str());
    if (!warn.empty()) {
        std::cout << "warn : " << warn << std::endl;
    }
    if (!ret) {
        if (!err.empty()) {
            std::cerr << err << std::endl;
        }
        return EXIT_FAILURE;
    }
    //loader.WriteGltfSceneToFile(&model, output_filename);

    // Embedd buffers and images
#ifndef TINYGLTF_NO_STB_IMAGE_WRITE
    loader.WriteGltfSceneToFile(&modelOK, embedded_filename, true, true);
#endif
    //main1();
    return EXIT_SUCCESS;
}