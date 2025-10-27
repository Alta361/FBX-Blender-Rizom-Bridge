#include <fbxsdk.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <cstdint>

// ============================================================================
// HELPERS
// ============================================================================

void WriteString(std::ofstream& out, const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) {
        out.write(str.c_str(), len);
    }
}

void WriteBlob(std::ofstream& out, const FbxBlob& blob) {
    uint32_t size = static_cast<uint32_t>(blob.Size());
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    if (size > 0) {
        out.write(static_cast<const char*>(blob.Access()), size);
    }
}

void WriteIntArray(std::ofstream& out, const std::vector<int>& arr) {
    uint32_t count = static_cast<uint32_t>(arr.size());
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    if (count > 0) {
        out.write(reinterpret_cast<const char*>(arr.data()), count * sizeof(int));
    }
}

void ProcessAndWriteProperty(std::ofstream& outFile, const FbxProperty& prop,
    const std::string& objectName, char marker) {
    std::string propName = prop.GetName().Buffer();
    std::cout << "Found property '" << propName << "' on object: " << objectName << std::endl;
    outFile.write(&marker, sizeof(marker));
    WriteString(outFile, objectName);
    WriteString(outFile, propName);
    std::string typeName = prop.GetPropertyDataType().GetName();
    WriteString(outFile, typeName);

    if (prop.GetPropertyDataType().Is(FbxIntDT)) {
        int val = prop.Get<int>();
        outFile.write(reinterpret_cast<const char*>(&val), sizeof(val));
        std::cout << " -> Saved " << sizeof(val) << " bytes (Int)." << std::endl;
    }
    else if (prop.GetPropertyDataType().Is(FbxBlobDT)) {
        FbxBlob val = prop.Get<FbxBlob>();
        WriteBlob(outFile, val);
        std::cout << " -> Saved " << val.Size() << " bytes (Blob)." << std::endl;
    }
    else if (prop.GetPropertyDataType().Is(FbxStringDT) ||
        prop.GetPropertyDataType().Is(FbxUrlDT)) {
        FbxString val = prop.Get<FbxString>();
        WriteString(outFile, val.Buffer());
        std::cout << " -> Saved " << val.GetLen() << " bytes (String)." << std::endl;
    }
}

// ============================================================================
// EXTRACTION from FbxDocument
// ============================================================================

void ExtractDocumentRizomData(FbxScene* scene, std::ofstream& outFile) {
    std::cout << "\n=== Extracting RizomUV data from FbxDocument ===" << std::endl;
    FbxDocument* rootDocument = scene->GetRootDocument();
    if (!rootDocument) return;

    FbxProperty rizomProp = rootDocument->FindProperty("RizomUV");
    if (rizomProp.IsValid()) {
        ProcessAndWriteProperty(outFile, rizomProp, "FbxDocument", 'G');

        FbxProperty sceneProp = rizomProp.Find("Scene");
        if (sceneProp.IsValid()) {
            ProcessAndWriteProperty(outFile, sceneProp, "FbxDocument", 'G');
        }

        FbxProperty uvSetsProp = rizomProp.Find("UVSets");
        if (uvSetsProp.IsValid()) {
            ProcessAndWriteProperty(outFile, uvSetsProp, "FbxDocument", 'G');

            FbxProperty uvMapProp = uvSetsProp.Find("UVMap");
            if (uvMapProp.IsValid()) {
                ProcessAndWriteProperty(outFile, uvMapProp, "FbxDocument", 'G');

                FbxProperty rootGroupProp = uvMapProp.Find("RootGroup");
                if (rootGroupProp.IsValid()) {
                    ProcessAndWriteProperty(outFile, rootGroupProp, "FbxDocument", 'G');
                }
            }
        }
    }
}

// ============================================================================
// EXTRACTION FROM GEOMETRY
// ============================================================================

void ExtractGeometryRizomData(FbxScene* scene, std::ofstream& outFile) {
    std::cout << "\n=== Extracting RizomUV data from Geometries ===" << std::endl;

    int nodeCount = scene->GetNodeCount();
    for (int i = 0; i < nodeCount; i++) {
        FbxNode* node = scene->GetNode(i);
        FbxNodeAttribute* attr = node->GetNodeAttribute();

        if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
            FbxMesh* mesh = node->GetMesh();

            
            std::string nodeName = node->GetName();
            std::string meshName = mesh->GetName();

            std::cout << "\nChecking node: " << nodeName << " (mesh: " << meshName << ")" << std::endl;

            
            std::string cacheName = nodeName;
            std::cout << "Processing geometry: " << cacheName << std::endl;

            // === Part 1: Property RizomUV ===
            FbxProperty rizomProp = mesh->FindProperty("RizomUV");
            if (rizomProp.IsValid()) {
                ProcessAndWriteProperty(outFile, rizomProp, cacheName, 'M');
            }

            FbxProperty uvSetsProp = mesh->FindProperty("RizomUVUVSets");
            if (uvSetsProp.IsValid()) {
                ProcessAndWriteProperty(outFile, uvSetsProp, cacheName, 'M');
            }

            // === Part 2: Island Group IDs ===
            int layerCount = mesh->GetLayerCount();
            for (int layerIndex = 0; layerIndex < layerCount; layerIndex++) {
                FbxLayer* layer = mesh->GetLayer(layerIndex);
                FbxLayerElementUserData* userData = layer->GetUserData();

                if (userData) {
                    std::string userDataName = userData->GetName();
                    std::cout << " Found UserData: '" << userDataName << "'" << std::endl;

                    bool isRizomData = (userDataName.find("Island") != std::string::npos) ||
                        (userDataName.find("RizomUV") != std::string::npos) ||
                        (userDataName.find("GroupID") != std::string::npos);

                    if (isRizomData) {
                        std::cout << " >>> Extracting UserData (contains RizomUV/Island/GroupID) <<<" << std::endl;

                        char marker = 'I';
                        outFile.write(&marker, sizeof(marker));
                        WriteString(outFile, cacheName);  
                        WriteString(outFile, userDataName);

                        std::vector<int> groupIDs;
                        int arrayCount = userData->GetDirectArrayCount();

                        if (arrayCount > 0) {
                            bool getStatus = false;
                            FbxLayerElementArrayTemplate<void*>* voidArray = userData->GetDirectArrayVoid(0, &getStatus);

                            if (getStatus && voidArray) {
                                int elementCount = voidArray->GetCount();
                                int* dataPtr = (int*)voidArray->GetLocked(FbxLayerElementArray::eReadLock);

                                if (dataPtr) {
                                    for (int k = 0; k < elementCount; k++) {
                                        groupIDs.push_back(dataPtr[k]);
                                    }
                                    voidArray->Release((void**)&dataPtr);
                                }
                            }
                        }

                        if (!groupIDs.empty()) {
                            WriteIntArray(outFile, groupIDs);
                            std::cout << " >>> Saved " << groupIDs.size() << " Island Group IDs <<<" << std::endl;
                        }
                        else {
                            WriteIntArray(outFile, groupIDs);
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: program.exe <input.fbx> <output.dat>\n";
        return 1;
    }

    const char* inputFBX = argv[1];
    const char* outputDAT = argv[2];

    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    FbxImporter* importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize(inputFBX, -1, manager->GetIOSettings())) {
        std::cerr << "Could not open FBX file: " << importer->GetStatus().GetErrorString() << std::endl;
        return 1;
    }

    FbxScene* scene = FbxScene::Create(manager, "Scene");
    importer->Import(scene);
    importer->Destroy();

    std::ofstream outFile(outputDAT, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Could not open output file: " << outputDAT << std::endl;
        return 1;
    }

    ExtractDocumentRizomData(scene, outFile);
    ExtractGeometryRizomData(scene, outFile);

    outFile.close();
    manager->Destroy();

    std::cout << "\n============================================" << std::endl;
    std::cout << "SUCCESS! Complete extraction finished." << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}