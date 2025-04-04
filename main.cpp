#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <iostream>
#include <vector>
#include <array>

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
    std::cout << str << std::endl; \
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

#define Error(x)\
{\
    Log(x); \
    exit(0); \
}

// Function to write a PNG image to a memory buffer using stb_image_write
unsigned char* WritePngToBuffer(int width, int height, int channels, const unsigned char* data, int& out_len) {
    // The stride_in_bytes parameter is the number of bytes between the start of one row and the start of the next row.
    // If the rows are tightly packed, you can set it to width * channels.
    int stride_in_bytes = width * channels;

    // Write the PNG image to a memory buffer
    unsigned char* png_buffer = stbi_write_png_to_mem(data, stride_in_bytes, width, height, channels, &out_len);
    if (png_buffer) {
        Log("Successfully wrote PNG image to buffer" << std::endl);
    } else {
        Error("Failed to write PNG image to buffer" << std::endl);
    }
    return png_buffer;
}

// Function to load a texture using stb_image
unsigned char* LoadTextureData(const std::string& filePath, int& width, int& height, int& channels) {
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        Error("Failed to load texture: " << filePath << std::endl);
    }
    return data;
}

// copy image data to global buffer and create BufferView for an image
void handleImage(tinygltf::Model& model, tinygltf::Image& image, unsigned char* data) {

    // write png format to intermediate buffer
    int out_len;
    unsigned char* png_buffer = WritePngToBuffer(image.width, image.height, image.component, data, out_len);

    // copy png data to global buffer and create tinygltf::BufferView for the image
    tinygltf::Buffer &buffer = model.buffers[0];
    size_t imageDataSize = out_len; //image.width * image.height * image.component;
    size_t bufferDataSize = buffer.data.size();
    buffer.data.resize( bufferDataSize + imageDataSize);
    std::copy(png_buffer, png_buffer + imageDataSize, buffer.data.begin() + bufferDataSize);
    stbi_image_free(data);
    stbi_image_free(png_buffer);

    tinygltf::BufferView bufferView{};
    bufferView.buffer = 0;
    bufferView.byteOffset = bufferDataSize;
    bufferView.byteLength = imageDataSize;
    bufferView.target = 0; // No specific target

    // Add the buffer view to the model
    model.bufferViews.push_back(bufferView);

    // Set the buffer view for the image
    image.bufferView = static_cast<int>(model.bufferViews.size() - 1);
    image.mimeType = "image/png"; // Set the appropriate MIME type
}

// Function to create a tinygltf texture by loading from file
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
    handleImage(model, image, data);

    // do not set uri, as we are embedding the image
    //image.uri = filePath;

    // Add the image to the model
    model.images.push_back(image);

    // Create a tinygltf::Texture
    tinygltf::Texture texture{};
    texture.source = static_cast<int>(model.images.size() - 1);
    model.textures.push_back(texture);

    return texture;
}

// Function to create a tinygltf texture by using in-memory data
tinygltf::Texture CreateTinyGltfTexture(tinygltf::Model& model, tinygltf::Image& metallicRoughnessImage, unsigned char* data) {

    handleImage(model, metallicRoughnessImage, data);

    // Add the image to the model
    model.images.push_back(metallicRoughnessImage);

    // Create a tinygltf::Texture
    tinygltf::Texture texture{};
    texture.source = static_cast<int>(model.images.size() - 1);
    model.textures.push_back(texture);

    return texture;
}

// Function to add PBR textures to tinygltf material
void AddTexturesToMaterial(tinygltf::Model& model, const std::vector<std::string>& mapFiles, tinygltf::Image& metallicRoughnessImage, unsigned char* data) {
    assert(model.materials.size() == 1);
    tinygltf::Material& material = model.materials[0];
    tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;

    for (const auto& filePath : mapFiles) {
        if (filePath.find("Color") != std::string::npos) {
            tinygltf::Texture texture = CreateTinyGltfTexture(model, filePath);
            pbr.baseColorTexture.index = static_cast<int>(model.textures.size() - 1);
        } else if (filePath.find("Normal") != std::string::npos) {
            tinygltf::Texture texture = CreateTinyGltfTexture(model, filePath);
            material.normalTexture.index = static_cast<int>(model.textures.size() - 1);
        } else if (filePath.find("AmbientOcclusion") != std::string::npos) {
            tinygltf::Texture texture = CreateTinyGltfTexture(model, filePath);
            material.occlusionTexture.index = static_cast<int>(model.textures.size() - 1);
        }
    }
    // now add merged metallic/rough texture:
    tinygltf::Texture texture = CreateTinyGltfTexture(model, metallicRoughnessImage, data);
    pbr.roughnessFactor = 1.0;
    pbr.metallicRoughnessTexture.index = static_cast<int>(model.textures.size() - 1);
}

// Function to merge roughness and metalness maps into a single metallicRoughness map
// we get grayscale info from each map and copy from the roughness map into the green channel of the combined map,
// and from the metalness map into the blue channel of the combined map.
// Red and alpha are all set to 255
void MergeRoughnessAndMetalness(const std::vector<std::string>& mapFiles, tinygltf::Image& metallicRoughnessImage, unsigned char** data_ptr_addr) {
    std::string metalnessPath;
    std::string roughnessPath;

    for (const auto& filePath : mapFiles) {
        if (filePath.find("Roughness") != std::string::npos) {
            roughnessPath = filePath;
        } else if (filePath.find("Metalness") != std::string::npos) {
            metalnessPath = filePath;
        }
    }
    if (roughnessPath.empty() || metalnessPath.empty()) {
        Error("ERROR: Roughness or Metalness map not found" << std::endl);
    }
    int width, height, channels;
    // load metalness grayscale image
    unsigned char* metalnessData = stbi_load(metalnessPath.c_str(), &width, &height, &channels, 1);
    if (!metalnessData) {
        Error("Failed to load metalness texture: " << metalnessPath << std::endl);
    }

    int roughnessWidth, roughnessHeight, roughnessChannels;
    // load roughness grayscale image
    unsigned char* roughnessData = stbi_load(roughnessPath.c_str(), &roughnessWidth, &roughnessHeight, &roughnessChannels, 1);
    if (!roughnessData) {
        Error("Failed to load roughness texture: " << roughnessPath << std::endl);
    }

    if (width != roughnessWidth || height != roughnessHeight) {
        Error("Metalness and roughness textures must have the same dimensions." << std::endl);
    }

    // load metalness again, this time as RGBA, where we want to store the merged image
    unsigned char* metallicRoughnessData = stbi_load(metalnessPath.c_str(), &width, &height, &channels, 4);
    if (!metallicRoughnessData) {
        Error("Failed to load metalness texture: " << metalnessPath << std::endl);
    }


    for (int i = 0; i < width * height; ++i) {
        metallicRoughnessData[i * 4 + 0] = 255; // Red channel
        metallicRoughnessData[i * 4 + 1] = roughnessData[i]; // Green channel (roughness)
        metallicRoughnessData[i * 4 + 2] = metalnessData[i]; // Blue channel (metalness)
        metallicRoughnessData[i * 4 + 3] = 255; // Alpha channel
    }

    stbi_image_free(roughnessData);
    stbi_image_free(metalnessData);

    // fill texture info
    metallicRoughnessImage.width = width;
    metallicRoughnessImage.height = height;
    metallicRoughnessImage.component = 4;
    metallicRoughnessImage.bits = 8;
    metallicRoughnessImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
    metallicRoughnessImage.image.resize(width * height * 4);
    *data_ptr_addr = metallicRoughnessData;
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

void extractIndexAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<unsigned char>& outData, int& stride,
    std::vector<double>& min, std::vector<double>& max,
    tinygltf::Accessor& accessorOut, tinygltf::BufferView& bufferViewOut) {
    auto& accessor = model.accessors[primitive.indices];
    auto& bufferView = model.bufferViews[accessor.bufferView];
    const unsigned char* bufferData = reinterpret_cast<const unsigned char*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
    outData.assign(bufferData, bufferData + accessor.count * 4);
    for (double minVal : accessor.minValues) {
        min.push_back(minVal);
    }
    for (double maxVal : accessor.maxValues) {
        max.push_back(maxVal);
    }
    bufferViewOut.target = bufferView.target;
    accessorOut.componentType = accessor.componentType;
    accessorOut.count = accessor.count;
    accessorOut.type = accessor.type;
}

void extractVertexAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& attributeName, std::vector<float>& outData, int& stride,
    std::vector<double>& min, std::vector<double>& max) {
    auto it = primitive.attributes.find(attributeName);
    if (it != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model.accessors[it->second];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        const float* bufferData = reinterpret_cast<const float*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
        stride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(accessor.type);
        outData.assign(bufferData, bufferData + accessor.count * stride);
        for (double minVal : accessor.minValues) {
            min.push_back(minVal);
        }
        for (double maxVal : accessor.maxValues) {
            max.push_back(maxVal);
        }
    }
}

// create model from data of the exported WorldCreator mesh
void createModel(tinygltf::Model& m, tinygltf::Model& modelMesh) {
    tinygltf::Scene scene;
    tinygltf::Mesh mesh;
    tinygltf::Primitive primitive;
    tinygltf::Node node;
    tinygltf::Asset asset;

    auto& primMesh = modelMesh.meshes[0].primitives[0];
    // Extract positions
    std::vector<float> positions;
    std::vector<double> posMin;
    std::vector<double> posMax;
    int posStrideMesh;
    extractVertexAttribute(modelMesh, primMesh, "POSITION", positions, posStrideMesh, posMin, posMax);
    Log(" copy mesh buffer positions: " << positions.size()/posStrideMesh << std::endl);
    // Extract normals
    std::vector<float> normals;
    std::vector<double> normalMin;
    std::vector<double> normalMax;
    int normalStride;
    extractVertexAttribute(modelMesh, primMesh, "NORMAL", normals, normalStride, normalMin, normalMax);
    Log(" copy mesh buffer normals: " << normals.size() / normalStride << std::endl);
    // Extract texture coordinates
    std::vector<float> texCoords;
    std::vector<double> texMin;
    std::vector<double> texMax;
    int texCoordStride;
    extractVertexAttribute(modelMesh, primMesh, "TEXCOORD_0", texCoords, texCoordStride, texMin, texMax);
    Log(" copy mesh buffer tex 0: " << texCoords.size() / texCoordStride << std::endl);

    // indices
    tinygltf::Accessor indicesAccessor;
    tinygltf::BufferView indicesBufferView;
    // Extract normals
    std::vector<unsigned char> indices; // typeless buffer
    std::vector<double> indicesMin;
    std::vector<double> indicesMax;
    int indicesStride;
    extractIndexAttribute(modelMesh, primMesh, indices, indicesStride, indicesMin, indicesMax, indicesAccessor, indicesBufferView);

    // create buffer from the input data
    tinygltf::Buffer buffer;
    // store buffer sizes
    size_t ps, is, ns, ts;
    ps = positions.size() * sizeof(float);
    is = indices.size(); // we handle this as bytes
    ns = normals.size() * sizeof(float);
    ts = texCoords.size() * sizeof(float);
    size_t totalBufferLength = ps + is + ns + ts;
    buffer.data.resize(totalBufferLength);
    std::memcpy(buffer.data.data(), positions.data(), ps);
    std::memcpy(buffer.data.data() + ps, indices.data(), is);
    std::memcpy(buffer.data.data() + ps + is, normals.data(), ns);
    std::memcpy(buffer.data.data() + ps + is + ns, texCoords.data(), ts);
    m.buffers.push_back(buffer);

    // define the buffer views
    tinygltf::BufferView positionBufferView;
    positionBufferView.buffer = 0;
    positionBufferView.byteOffset = 0;
    positionBufferView.byteLength = ps;
    positionBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    m.bufferViews.push_back(positionBufferView);

    indicesBufferView.buffer = 0;
    indicesBufferView.byteOffset = ps;
    indicesBufferView.byteLength = is;
    //indicesBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER; set in extract method
    m.bufferViews.push_back(indicesBufferView);

    tinygltf::BufferView normalBufferView;
    normalBufferView.buffer = 0;
    normalBufferView.byteOffset = ps + is;
    normalBufferView.byteLength = ns;
    normalBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    m.bufferViews.push_back(normalBufferView);

    tinygltf::BufferView texcoordBufferView;
    texcoordBufferView.buffer = 0;
    texcoordBufferView.byteOffset = ps + is + ns;
    texcoordBufferView.byteLength = ts;
    texcoordBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    m.bufferViews.push_back(texcoordBufferView);

    // define the accessors

    tinygltf::Accessor positionAccessor;
    positionAccessor.bufferView = 0;
    positionAccessor.byteOffset = 0;
    positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    positionAccessor.count = static_cast<int>(positions.size() / 3);
    positionAccessor.type = TINYGLTF_TYPE_VEC3;
    positionAccessor.maxValues = posMax;
    positionAccessor.minValues = posMin;
    m.accessors.push_back(positionAccessor);

    indicesAccessor.bufferView = 1;
    indicesAccessor.byteOffset = 0;
    //indicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT; // set in extract method, also count and type
    // do NOT write min/max for index buffer:
    //indicesAccessor.maxValues = indicesMax;
    //indicesAccessor.minValues = indicesMin;
    m.accessors.push_back(indicesAccessor);

    tinygltf::Accessor normalAccessor;
    normalAccessor.bufferView = 2;
    normalAccessor.byteOffset = 0;
    normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    normalAccessor.count = static_cast<int>(normals.size() / 3);
    //normalAccessor.count = static_cast<int>(3); // override
    normalAccessor.type = TINYGLTF_TYPE_VEC3;
    normalAccessor.maxValues = normalMax;
    normalAccessor.minValues = normalMin;
    m.accessors.push_back(normalAccessor);

    tinygltf::Accessor texcoordAccessor;
    texcoordAccessor.bufferView = 3;
    texcoordAccessor.byteOffset = 0;
    texcoordAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    texcoordAccessor.count = static_cast<int>(texCoords.size() / 2);
    texcoordAccessor.type = TINYGLTF_TYPE_VEC2;
    texcoordAccessor.maxValues = texMax;
    texcoordAccessor.minValues = texMin;
    m.accessors.push_back(texcoordAccessor);

    // Build the mesh primitive and add it to the mesh
    primitive.attributes["POSITION"] = 0;
    primitive.indices = 1;
    primitive.attributes["NORMAL"] = 2;
    primitive.attributes["TEXCOORD_0"] = 3;
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
    mat.pbrMetallicRoughness.baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    //mat.doubleSided = true; may need to be set for off cases
    m.materials.push_back(mat);
}

void usage() {
    Log("Usage: WorldCreatorToGLTF <BaseName> <InputFolder> [<OutputFolder>]" << std::endl);
    Log("  typical use: WorldCreatorToGLTF Terrain c:/export " << std::endl);
    Log("    will produce Terrain.glb in folder c:/export " << std::endl);
}

void setup(const std::string baseName, const std::string inputFolder, const std::string outputFolder, std::vector<std::string>& texFiles, std::string& meshFile) {
    if (!std::filesystem::exists(inputFolder)) {
        Error("Input folder does not exist: " << inputFolder << std::endl);
    }
    if (!std::filesystem::exists(outputFolder)) {
        Error("Output folder does not exist: " << outputFolder << std::endl);
    }
    for (const auto& entry : std::filesystem::directory_iterator(inputFolder)) {
        if (entry.is_regular_file()) {
            const std::string fileName = entry.path().filename().string();
            if (fileName.find(baseName) != std::string::npos && fileName.find(".png") != std::string::npos) {
                texFiles.push_back(entry.path().string());
                Log("Found texture file: " << fileName << std::endl);
            }
        }
    }
    if (texFiles.size() == 0) {
        Error("No texture files found in input folder for base name " << baseName << std::endl);
    }
    for (const auto& entry : std::filesystem::directory_iterator(inputFolder)) {
        if (entry.is_regular_file()) {
            const std::string fileName = entry.path().filename().string();
            if (fileName.find(baseName) != std::string::npos && fileName.find("Mesh") != std::string::npos) {
                meshFile = entry.path().string();
                Log("Found mesh file: " << fileName << std::endl);
            }
        }
    }
    if (meshFile.empty()) {
        Error("No mesh file found in input folder for base name " << baseName << std::endl);
    }
}

int main(int argc, char* argv[]) {
    Log(argv[0] << std::endl);
    if (argc != 4 && argc != 3) {
        usage();
        exit(1);
    }

    std::vector<std::string> texFiles;
    std::string meshFile;
    std::string baseName = argv[1];
    std::string inputFolder = argv[2];
    std::string outputFolder = argc == 4 ? argv[3] : inputFolder;
    setup(baseName, inputFolder, outputFolder, texFiles, meshFile);

    tinygltf::Model modelMesh;
    tinygltf::Model model;
    if (true) {
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        // assume ascii glTF.
        bool ret = loader.LoadBinaryFromFile(&modelMesh, &err, &warn, meshFile.c_str());
        if (!warn.empty()) {
            std::cout << "warn : " << warn << std::endl;
        }
        if (!ret) {
            if (!err.empty()) {
                std::cerr << err << std::endl;
            }
            return EXIT_FAILURE;
        }
        Log("Loaded mesh model" << std::endl);
    }
    createModel(model, modelMesh);
    tinygltf::Image metallicRoughnessImage; // partly filled, not put to gltf model
    unsigned char* metallicRoughnessData = nullptr;
    MergeRoughnessAndMetalness(texFiles, metallicRoughnessImage, &metallicRoughnessData);

    AddTexturesToMaterial(model, texFiles, metallicRoughnessImage, metallicRoughnessData);

    //if (!ValidateTinyGltfModel(model)) {
    //    Log("Model validation failed" << std::endl);
    //    return -1;
    //}

    // Construct the output file path
    std::string nameWithType = baseName + ".glb";
    std::filesystem::path outputPath = std::filesystem::path(outputFolder) / nameWithType;
    // Save it to a file
    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(&model, outputPath.string(),
        false, // embedImages
        true, // embedBuffers
        false, // pretty print
        true); // write binary

    Log("Exported successfully to " << outputPath << std::endl);

    return 0;
}
