#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Mesh.h"

class Model {
public:
    bool LoadFromOBJ(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[Model] Failed to open OBJ file: " << path << std::endl;
            return false;
        }

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> texcoords;
        std::vector<glm::vec3> normals;

        std::vector<MeshBuildData> buildMeshes;
        MeshBuildData currentMesh;
        currentMesh.Name = "default";

        std::string currentMaterialName = "default";
        std::string line;

        while (std::getline(file, line)) {
            StripComment(line);
            Trim(line);
            if (line.empty()) {
                continue;
            }

            std::istringstream stream(line);
            std::string head;
            stream >> head;

            if (head == "v") {
                glm::vec3 p(0.0f);
                stream >> p.x >> p.y >> p.z;
                positions.push_back(p);
                continue;
            }

            if (head == "vt") {
                glm::vec2 uv(0.0f);
                stream >> uv.x >> uv.y;
                texcoords.push_back(uv);
                continue;
            }

            if (head == "vn") {
                glm::vec3 n(0.0f, 1.0f, 0.0f);
                stream >> n.x >> n.y >> n.z;
                if (glm::length(n) > 1e-6f) {
                    n = glm::normalize(n);
                }
                normals.push_back(n);
                continue;
            }

            if (head == "o" || head == "g") {
                std::string objectName;
                stream >> objectName;
                if (objectName.empty()) {
                    objectName = "unnamed";
                }
                FlushCurrentMesh(buildMeshes, currentMesh, currentMaterialName);
                currentMesh = MeshBuildData();
                currentMesh.Name = objectName;
                currentMaterialName = "default";
                continue;
            }

            if (head == "usemtl") {
                std::string materialName;
                stream >> materialName;
                if (materialName.empty()) {
                    materialName = "default";
                }
                FlushCurrentMesh(buildMeshes, currentMesh, currentMaterialName);
                currentMesh = MeshBuildData();
                currentMesh.Name = "submesh";
                currentMaterialName = materialName;
                continue;
            }

            if (head == "f") {
                std::vector<std::string> faceTokens;
                std::string token;
                while (stream >> token) {
                    faceTokens.push_back(token);
                }

                if (faceTokens.size() < 3) {
                    continue;
                }

                std::vector<unsigned int> faceIndices;
                faceIndices.reserve(faceTokens.size());

                for (const std::string& faceToken : faceTokens) {
                    ObjVertexRef ref;
                    if (!ParseFaceToken(faceToken, ref)) {
                        continue;
                    }

                    unsigned int idx = AddVertex(ref,
                                                 positions,
                                                 texcoords,
                                                 normals,
                                                 currentMesh);
                    faceIndices.push_back(idx);
                }

                if (faceIndices.size() < 3) {
                    continue;
                }

                for (std::size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                    currentMesh.Indices.push_back(faceIndices[0]);
                    currentMesh.Indices.push_back(faceIndices[i]);
                    currentMesh.Indices.push_back(faceIndices[i + 1]);
                }
            }
        }

        FlushCurrentMesh(buildMeshes, currentMesh, currentMaterialName);

        std::vector<Mesh> loadedMeshes;
        loadedMeshes.reserve(buildMeshes.size());

        for (MeshBuildData& build : buildMeshes) {
            if (build.Vertices.empty() || build.Indices.empty()) {
                continue;
            }

            ComputeNormalsAndTangents(build.Vertices, build.Indices);

            MeshMaterial material;
            material.Name = build.MaterialName;
            material.AlbedoTint = ColorFromName(build.MaterialName);

            loadedMeshes.emplace_back(std::move(build.Vertices),
                                      std::move(build.Indices),
                                      std::move(material));
        }

        if (loadedMeshes.empty()) {
            std::cerr << "[Model] OBJ parsed, but no valid mesh data: " << path << std::endl;
            return false;
        }

        m_SourcePath = path;
        m_Meshes = std::move(loadedMeshes);

        std::cout << "[Model] Loaded OBJ: " << path
                  << " (meshes: " << m_Meshes.size() << ")" << std::endl;
        return true;
    }

    static Model CreateFallbackModel()
    {
        Model model;

        std::vector<MeshVertex> vertices = {
            {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.0f, 0.8f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}}
        };

        std::vector<unsigned int> indices = {
            0, 1, 2, 0, 2, 3,
            0, 3, 4,
            3, 2, 4,
            2, 1, 4,
            1, 0, 4
        };

        MeshMaterial material;
        material.Name = "fallback";
        material.AlbedoTint = glm::vec3(0.9f, 0.7f, 0.4f);

        model.m_Meshes.emplace_back(std::move(vertices), std::move(indices), std::move(material));
        model.m_SourcePath = "fallback";
        return model;
    }

    void Draw(const Shader& shader, const glm::mat4& modelMatrix = glm::mat4(1.0f)) const
    {
        for (const Mesh& mesh : m_Meshes) {
            mesh.Draw(shader, modelMatrix);
        }
    }

    void DrawInstanced(const Shader& shader,
                       const std::vector<glm::mat4>& instanceMatrices,
                       const glm::mat4& baseModel = glm::mat4(1.0f))
    {
        for (Mesh& mesh : m_Meshes) {
            mesh.DrawInstanced(shader, instanceMatrices, baseModel);
        }
    }

    std::size_t GetMeshCount() const { return m_Meshes.size(); }
    const std::string& GetSourcePath() const { return m_SourcePath; }

private:
    struct ObjVertexRef {
        int Position = 0;
        int TexCoord = 0;
        int Normal = 0;

        bool operator==(const ObjVertexRef& other) const
        {
            return Position == other.Position &&
                   TexCoord == other.TexCoord &&
                   Normal == other.Normal;
        }
    };

    struct ObjVertexRefHash {
        std::size_t operator()(const ObjVertexRef& ref) const
        {
            std::size_t h1 = std::hash<int>()(ref.Position);
            std::size_t h2 = std::hash<int>()(ref.TexCoord);
            std::size_t h3 = std::hash<int>()(ref.Normal);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    struct MeshBuildData {
        std::string Name;
        std::string MaterialName;
        std::vector<MeshVertex> Vertices;
        std::vector<unsigned int> Indices;
        std::unordered_map<ObjVertexRef, unsigned int, ObjVertexRefHash> VertexToIndex;
    };

    static void StripComment(std::string& line)
    {
        const std::size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
    }

    static void Trim(std::string& text)
    {
        auto notSpace = [](unsigned char c) { return !std::isspace(c); };

        auto first = std::find_if(text.begin(), text.end(), notSpace);
        if (first == text.end()) {
            text.clear();
            return;
        }

        auto last = std::find_if(text.rbegin(), text.rend(), notSpace).base();
        text = std::string(first, last);
    }

    static void FlushCurrentMesh(std::vector<MeshBuildData>& output,
                                 MeshBuildData& current,
                                 const std::string& materialName)
    {
        if (current.Indices.empty()) {
            return;
        }

        current.MaterialName = materialName;
        output.push_back(std::move(current));
    }

    static bool ParseFaceToken(const std::string& token, ObjVertexRef& outRef)
    {
        if (token.empty()) {
            return false;
        }

        std::array<std::string, 3> parts;
        std::size_t start = 0;
        int partIndex = 0;

        for (std::size_t i = 0; i <= token.size() && partIndex < 3; ++i) {
            if (i == token.size() || token[i] == '/') {
                parts[partIndex++] = token.substr(start, i - start);
                start = i + 1;
            }
        }

        outRef.Position = ParseIndexPart(parts[0]);
        outRef.TexCoord = ParseIndexPart(parts[1]);
        outRef.Normal = ParseIndexPart(parts[2]);

        return outRef.Position != 0;
    }

    static int ParseIndexPart(const std::string& text)
    {
        if (text.empty()) {
            return 0;
        }

        return std::atoi(text.c_str());
    }

    static int ResolveOBJIndex(int rawIndex, int itemCount)
    {
        if (rawIndex > 0) {
            return rawIndex - 1;
        }

        if (rawIndex < 0) {
            return itemCount + rawIndex;
        }

        return -1;
    }

    static unsigned int AddVertex(const ObjVertexRef& rawRef,
                                  const std::vector<glm::vec3>& positions,
                                  const std::vector<glm::vec2>& texcoords,
                                  const std::vector<glm::vec3>& normals,
                                  MeshBuildData& build)
    {
        ObjVertexRef ref;
        ref.Position = ResolveOBJIndex(rawRef.Position, static_cast<int>(positions.size()));
        ref.TexCoord = ResolveOBJIndex(rawRef.TexCoord, static_cast<int>(texcoords.size()));
        ref.Normal = ResolveOBJIndex(rawRef.Normal, static_cast<int>(normals.size()));

        const auto found = build.VertexToIndex.find(ref);
        if (found != build.VertexToIndex.end()) {
            return found->second;
        }

        MeshVertex vertex;
        if (ref.Position >= 0 && ref.Position < static_cast<int>(positions.size())) {
            vertex.Position = positions[ref.Position];
        }

        if (ref.TexCoord >= 0 && ref.TexCoord < static_cast<int>(texcoords.size())) {
            vertex.TexCoord = texcoords[ref.TexCoord];
        }

        if (ref.Normal >= 0 && ref.Normal < static_cast<int>(normals.size())) {
            vertex.Normal = normals[ref.Normal];
        }

        const unsigned int newIndex = static_cast<unsigned int>(build.Vertices.size());
        build.Vertices.push_back(vertex);
        build.VertexToIndex.emplace(ref, newIndex);
        return newIndex;
    }

    static glm::vec3 BuildFallbackTangent(const glm::vec3& normal)
    {
        const glm::vec3 up = (std::abs(normal.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                           : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::cross(up, normal);
        if (glm::length(tangent) < 1e-6f) {
            tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        return glm::normalize(tangent);
    }

    static void ComputeNormalsAndTangents(std::vector<MeshVertex>& vertices,
                                          const std::vector<unsigned int>& indices)
    {
        if (vertices.empty() || indices.size() < 3) {
            return;
        }

        std::vector<glm::vec3> accumulatedNormals(vertices.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> accumulatedTangents(vertices.size(), glm::vec3(0.0f));

        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            const unsigned int i0 = indices[i];
            const unsigned int i1 = indices[i + 1];
            const unsigned int i2 = indices[i + 2];

            if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
                continue;
            }

            const glm::vec3& p0 = vertices[i0].Position;
            const glm::vec3& p1 = vertices[i1].Position;
            const glm::vec3& p2 = vertices[i2].Position;

            const glm::vec2& uv0 = vertices[i0].TexCoord;
            const glm::vec2& uv1 = vertices[i1].TexCoord;
            const glm::vec2& uv2 = vertices[i2].TexCoord;

            const glm::vec3 edge1 = p1 - p0;
            const glm::vec3 edge2 = p2 - p0;

            glm::vec3 faceNormal = glm::cross(edge1, edge2);
            if (glm::length(faceNormal) > 1e-6f) {
                faceNormal = glm::normalize(faceNormal);
                accumulatedNormals[i0] += faceNormal;
                accumulatedNormals[i1] += faceNormal;
                accumulatedNormals[i2] += faceNormal;
            }

            const glm::vec2 dUV1 = uv1 - uv0;
            const glm::vec2 dUV2 = uv2 - uv0;
            const float determinant = dUV1.x * dUV2.y - dUV2.x * dUV1.y;
            if (std::abs(determinant) > 1e-8f) {
                const float invDet = 1.0f / determinant;
                glm::vec3 tangent;
                tangent.x = invDet * (dUV2.y * edge1.x - dUV1.y * edge2.x);
                tangent.y = invDet * (dUV2.y * edge1.y - dUV1.y * edge2.y);
                tangent.z = invDet * (dUV2.y * edge1.z - dUV1.y * edge2.z);

                accumulatedTangents[i0] += tangent;
                accumulatedTangents[i1] += tangent;
                accumulatedTangents[i2] += tangent;
            }
        }

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            if (glm::length(vertices[i].Normal) < 1e-6f) {
                if (glm::length(accumulatedNormals[i]) > 1e-6f) {
                    vertices[i].Normal = glm::normalize(accumulatedNormals[i]);
                }
                else {
                    vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }
            else {
                vertices[i].Normal = glm::normalize(vertices[i].Normal);
            }

            glm::vec3 tangent = accumulatedTangents[i];
            tangent = tangent - vertices[i].Normal * glm::dot(vertices[i].Normal, tangent);

            if (glm::length(tangent) > 1e-6f) {
                vertices[i].Tangent = glm::normalize(tangent);
            }
            else {
                vertices[i].Tangent = BuildFallbackTangent(vertices[i].Normal);
            }
        }
    }

    static glm::vec3 ColorFromName(const std::string& name)
    {
        unsigned int hash = 2166136261u;
        for (char c : name) {
            hash ^= static_cast<unsigned char>(c);
            hash *= 16777619u;
        }

        const float r = 0.5f + 0.5f * static_cast<float>((hash & 0xFFu)) / 255.0f;
        const float g = 0.5f + 0.5f * static_cast<float>(((hash >> 8) & 0xFFu)) / 255.0f;
        const float b = 0.5f + 0.5f * static_cast<float>(((hash >> 16) & 0xFFu)) / 255.0f;

        return glm::vec3(r, g, b);
    }

private:
    std::vector<Mesh> m_Meshes;
    std::string m_SourcePath;
};

#endif