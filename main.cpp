#include <assimp/commonMetaData.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>
//#include <rapidjson/schema.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include <assimp/mesh.h>
#include <assimp/Logger.hpp>
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

// Example stream
class myStream : public Assimp::LogStream {
public:
    // Write something using your own functionality
    void write(const char* message) {
        //Log("" << message << std::endl);
    }
};

bool validateMeshPrimitives(const aiScene* scene) {
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
            Log("Mesh contains non-triangle primitives." << std::endl);
            return false;
        }
    }
    return true;
}

bool validate(const aiScene* scene) {
    if (!scene->HasMeshes() || !scene->HasMaterials()) {
        Log("Scene is missing essential components." << std::endl);
        return false;
    }
    return validateMeshPrimitives(scene);
}

int main() {
    // Create a logger instance
    Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);

    // Select the kinds of messages you want to receive on this log stream
    const unsigned int severity = Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn;

    // Attaching it to the default logger
    Assimp::DefaultLogger::get()->attachStream(new myStream, severity);    // Create a logger instance

    // Test log message
    Assimp::DefaultLogger::get()->info("Test log message");

    // Create a new scene
    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode();
    scene->mMaterials = new aiMaterial * [1];
    scene->mNumMaterials = 1;
    scene->mMeshes = new aiMesh * [1];
    scene->mNumMeshes = 1;
    scene->mRootNode->mMeshes = new unsigned int[1];
    scene->mRootNode->mNumMeshes = 1;

    // Create a mesh
    aiMesh* mesh = new aiMesh();
    mesh->mMaterialIndex = 0;
    mesh->mNumVertices = 3;
    mesh->mVertices = new aiVector3D[3];
    mesh->mVertices[0] = aiVector3D(0.0f, 0.0f, 0.0f);
    mesh->mVertices[1] = aiVector3D(1.0f, 0.0f, 0.0f);
    mesh->mVertices[2] = aiVector3D(0.0f, 1.0f, 0.0f);
    mesh->mNumFaces = 1;
    mesh->mFaces = new aiFace[1];
    mesh->mFaces[0].mNumIndices = 3;
    mesh->mFaces[0].mIndices = new unsigned int[3];
    mesh->mFaces[0].mIndices[0] = 0;
    mesh->mFaces[0].mIndices[1] = 1;
    mesh->mFaces[0].mIndices[2] = 2;
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    scene->mMeshes[0] = mesh;
    scene->mRootNode->mMeshes[0] = 0;

    // Create a material using the metallic-roughness model
    aiMaterial* material = new aiMaterial();
    aiColor4D baseColor(1.0f, 0.0f, 0.0f, 1.0f); // Red color
    material->AddProperty(&baseColor, 1, AI_MATKEY_COLOR_DIFFUSE);
    float metallic = 1.0f;
    material->AddProperty(&metallic, 1, AI_MATKEY_METALLIC_FACTOR);
    float roughness = 0.5f;
    material->AddProperty(&roughness, 1, AI_MATKEY_ROUGHNESS_FACTOR);
    scene->mMaterials[0] = material;

    if (!validate(scene)) {
        Log("Error validating scene" << std::endl);
        return -1;
    }
    // Export the scene to a glTF file
    Assimp::Exporter exporter;
    aiReturn result = exporter.Export(scene, "glb2", "output.glb");

    if (result != aiReturn_SUCCESS) {
        Log("Error exporting file: " << exporter.GetErrorString() << std::endl);
        return -1;
    }

    Log("Exported successfully to output.gltf" << std::endl);

    // Clean up
    delete scene;

    // Destroy the logger instance
    Assimp::DefaultLogger::kill();

    return 0;
}
