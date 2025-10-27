#include <fbxsdk.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <cstring>

// ============================================================================
// Helpers
// ============================================================================

bool ReadString(std::ifstream& in, std::string& str) {
    uint32_t len;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (!in) return false;
    str.resize(len);
    if (len > 0) {
        in.read(&str[0], len);
    }
    return !!in;
}

bool ReadIntArray(std::ifstream& in, std::vector<int>& arr) {
    uint32_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in) return false;
    arr.resize(count);
    if (count > 0) {
        in.read(reinterpret_cast<char*>(arr.data()), count * sizeof(int));
    }
    return !!in;
}

bool ReadValue(std::ifstream& in, const std::string& typeName, std::vector<char>& data) {
    if (typeName == "Int" || typeName == "Integer") {
        data.resize(sizeof(int));
        in.read(data.data(), sizeof(int));
    }
    else if (typeName == "Blob" || typeName == "KString" || typeName == "String") {
        uint32_t size;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in) return false;
        data.resize(size);
        if (size > 0) {
            in.read(data.data(), size);
        }
    }
    else {
        std::cerr << "\nError: Unknown property type '" << typeName << "'" << std::endl;
        return false;
    }
    return !!in;
}

// ============================================================================
// date Structure
// ============================================================================

struct GeometryData {
    std::map<std::string, std::pair<std::string, std::vector<char>>> properties;
    std::string userDataName;
    std::vector<int> islandGroupIDs;
    bool hasIslandData = false;
};

// ============================================================================
// Load Data form file
// ============================================================================

void LoadAllDataFromFile(const std::string& dataFilePath,
    std::map<std::string, std::pair<std::string, std::vector<char>>>& documentProperties,
    std::map<std::string, GeometryData>& geometryData) {

    std::ifstream dataFile(dataFilePath, std::ios::binary);
    if (!dataFile.is_open()) {
        std::cerr << "Error: Could not open data file.\n";
        return;
    }

    std::cout << "\n=== Loading data from file ===" << std::endl;

    while (dataFile) {
        char marker;
        dataFile.read(&marker, sizeof(marker));
        if (dataFile.eof()) break;

        if (marker == 'G') {
            std::string objectName, propName, typeName;
            ReadString(dataFile, objectName);
            ReadString(dataFile, propName);
            ReadString(dataFile, typeName);
            std::vector<char> data;
            if (!ReadValue(dataFile, typeName, data)) break;
            documentProperties[propName] = { typeName, data };
            std::cout << "  [Document] Property '" << propName << "' (" << data.size() << " bytes)" << std::endl;

        }
        else if (marker == 'M') {
            std::string meshName, propName, typeName;
            ReadString(dataFile, meshName);
            ReadString(dataFile, propName);
            ReadString(dataFile, typeName);
            std::vector<char> data;
            if (!ReadValue(dataFile, typeName, data)) break;
            geometryData[meshName].properties[propName] = { typeName, data };
            std::cout << "  [" << meshName << "] Property '" << propName << "' (" << data.size() << " bytes)" << std::endl;

        }
        else if (marker == 'I') {
            std::string meshName, userDataName;
            ReadString(dataFile, meshName);
            ReadString(dataFile, userDataName);
            std::vector<int> groupIDs;
            if (!ReadIntArray(dataFile, groupIDs)) break;
            geometryData[meshName].userDataName = userDataName;
            geometryData[meshName].islandGroupIDs = groupIDs;
            geometryData[meshName].hasIslandData = true;
            std::cout << "  [" << meshName << "] UserData '" << userDataName << "' (" << groupIDs.size() << " IDs)" << std::endl;
        }
    }
    dataFile.close();
}

// ============================================================================
// Inject FbxDocument
// ============================================================================

void InjectDocumentRizomData(FbxScene* scene,
    std::map<std::string, std::pair<std::string, std::vector<char>>>& properties) {

    std::cout << "\n=== Injecting RizomUV data into FbxDocument ===" << std::endl;

    FbxDocument* rootDocument = scene->GetRootDocument();
    if (!rootDocument) return;

    rootDocument->SetName("Scene");

    FbxProperty rizomProp = FbxProperty::Create(rootDocument, FbxIntDT, "RizomUV");
    if (rizomProp.IsValid() && properties.count("RizomUV")) {
        rizomProp.Set(*(reinterpret_cast<int*>(properties["RizomUV"].second.data())));
        std::cout << "  Created: RizomUV (int)" << std::endl;
    }

    FbxProperty sceneProp = FbxProperty::Create(rizomProp, FbxBlobDT, "Scene");
    if (sceneProp.IsValid() && properties.count("Scene")) {
        auto& data = properties["Scene"].second;
        sceneProp.Set(FbxBlob(data.data(), data.size()));
        std::cout << "  Created: RizomUV->Scene (blob, " << data.size() << " bytes)" << std::endl;
    }

    FbxProperty uvSetsProp = FbxProperty::Create(rizomProp, FbxStringDT, "UVSets");
    if (uvSetsProp.IsValid() && properties.count("UVSets")) {
        auto& data = properties["UVSets"].second;
        uvSetsProp.Set(FbxString(data.data(), data.size()));
        std::cout << "  Created: RizomUV->UVSets (string)" << std::endl;
    }

    FbxProperty uvMapProp = FbxProperty::Create(uvSetsProp, FbxStringDT, "UVMap");
    if (uvMapProp.IsValid() && properties.count("UVMap")) {
        auto& data = properties["UVMap"].second;
        uvMapProp.Set(FbxString(data.data(), data.size()));
        std::cout << "  Created: RizomUV->UVSets->UVMap (string)" << std::endl;
    }

    FbxProperty rootGroupProp = FbxProperty::Create(uvMapProp, FbxBlobDT, "RootGroup");
    if (rootGroupProp.IsValid() && properties.count("RootGroup")) {
        auto& data = properties["RootGroup"].second;
        rootGroupProp.Set(FbxBlob(data.data(), data.size()));
        std::cout << "  Created: RizomUV->UVSets->UVMap->RootGroup (blob, " << data.size() << " bytes)" << std::endl;
    }

    std::cout << "  SUCCESS: Document property hierarchy created." << std::endl;
}

// ============================================================================
// Inject Geometry
// ============================================================================

void InjectGeometryRizomData(FbxScene* scene, std::map<std::string, GeometryData>& geometryData) {
    std::cout << "\n=== Injecting RizomUV data into Geometries ===" << std::endl;

    int nodeCount = scene->GetNodeCount();
    for (int i = 0; i < nodeCount; i++) {
        FbxNode* node = scene->GetNode(i);
        FbxNodeAttribute* attr = node->GetNodeAttribute();

        if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
            FbxMesh* mesh = node->GetMesh();

            std::string nodeName = node->GetName();
            std::string meshName = mesh->GetName();

            std::cout << "\nChecking node: " << nodeName << " (mesh: " << meshName << ")" << std::endl;

            std::string lookupName;
            GeometryData* geoData = nullptr;

            if (geometryData.count(nodeName)) {
                lookupName = nodeName;
                geoData = &geometryData[nodeName];
                std::cout << "  OK Found data for NODE name: " << nodeName << std::endl;
            }
            else if (geometryData.count(meshName)) {
                lookupName = meshName;
                geoData = &geometryData[meshName];
                std::cout << "  OK Found data for MESH name: " << meshName << std::endl;
            }
            else {
                std::cout << "  -- No data found for '" << nodeName << "' or '" << meshName << "'" << std::endl;
                continue;
            }

            std::cout << "Processing geometry: " << lookupName << std::endl;

            // ===  1:  RizomUV Properties ===
            if (geoData->properties.count("RizomUV")) {
                FbxProperty rizomProp = FbxProperty::Create(mesh, FbxIntDT, "RizomUV");
                if (rizomProp.IsValid()) {
                    auto& data = geoData->properties["RizomUV"].second;
                    rizomProp.Set(*(reinterpret_cast<int*>(data.data())));
                    std::cout << "  Created: RizomUV (int)" << std::endl;
                }
            }

            if (geoData->properties.count("RizomUVUVSets")) {
                FbxProperty uvSetsProp = FbxProperty::Create(mesh, FbxStringDT, "RizomUVUVSets");
                if (uvSetsProp.IsValid()) {
                    auto& data = geoData->properties["RizomUVUVSets"].second;
                    uvSetsProp.Set(FbxString(data.data(), data.size()));
                    std::cout << "  Created: RizomUVUVSets (string)" << std::endl;
                }
            }

            // === 2: Island Group IDs ===
            if (geoData->hasIslandData && !geoData->islandGroupIDs.empty()) {
                FbxLayer* layer = mesh->GetLayer(0);
                if (!layer) {
                    mesh->CreateLayer();
                    layer = mesh->GetLayer(0);
                }

                std::string userDataName = geoData->userDataName.empty() ?
                    "RizomUVUVMapIslandGroupIDs" :
                    geoData->userDataName;

                std::cout << "  Creating UserData: '" << userDataName << "'" << std::endl;

                FbxArray<FbxDataType> dataTypes;
                dataTypes.Add(FbxIntDT);
                FbxArray<const char*> dataNames;
                dataNames.Add("IslandGroupID");

                FbxLayerElementUserData* userData = FbxLayerElementUserData::Create(
                    mesh,
                    userDataName.c_str(),
                    0,
                    dataTypes,
                    dataNames
                );

                if (userData) {
                    userData->SetMappingMode(FbxLayerElement::eByPolygon);
                    userData->SetReferenceMode(FbxLayerElement::eDirect);
                    userData->ResizeAllDirectArrays(geoData->islandGroupIDs.size());

                    bool getStatus = false;
                    FbxLayerElementArrayTemplate<void*>* voidArray = userData->GetDirectArrayVoid(0, &getStatus);

                    if (getStatus && voidArray) {
                        int* dataPtr = (int*)voidArray->GetLocked(FbxLayerElementArray::eWriteLock);

                        if (dataPtr) {
                            for (size_t j = 0; j < geoData->islandGroupIDs.size(); j++) {
                                dataPtr[j] = geoData->islandGroupIDs[j];
                            }

                            voidArray->Release((void**)&dataPtr);
                            std::cout << "  SAVED " << geoData->islandGroupIDs.size() << " Island Group IDs" << std::endl;
                        }
                    }

                    layer->SetUserData(userData);
                }
            }
        }
    }
    std::cout << "\n  SUCCESS: All geometry data injected." << std::endl;
}


int main(int argc, char** argv) {
    if (argc < 4) {
        std::cout << "Usage: program.exe <target.fbx> <data.dat> <output.fbx>\n";
        return 1;
    }

    const char* targetFBX = argv[1];
    const char* dataFile = argv[2];
    const char* outputFBX = argv[3];

    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    FbxImporter* importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize(targetFBX, -1, manager->GetIOSettings())) {
        std::cerr << "Could not open the target FBX file.\n";
        return 1;
    }

    FbxScene* scene = FbxScene::Create(manager, "Scene");
    importer->Import(scene);
    importer->Destroy();

    std::map<std::string, std::pair<std::string, std::vector<char>>> documentProperties;
    std::map<std::string, GeometryData> geometryData;

    LoadAllDataFromFile(dataFile, documentProperties, geometryData);
    InjectDocumentRizomData(scene, documentProperties);
    InjectGeometryRizomData(scene, geometryData);

    std::cout << "\n=== CONVERTING: ASCII -> BINARY ===" << std::endl;
    std::cout << "Saving to: " << outputFBX << std::endl;

    FbxExporter* exporter = FbxExporter::Create(manager, "");

    int fileFormat = manager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX binary (*.fbx)");

    if (fileFormat == -1) {
        std::cerr << "ERROR: FBX binary format not found! Falling back to ASCII." << std::endl;
        fileFormat = manager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
    }

    if (!exporter->Initialize(outputFBX, fileFormat, manager->GetIOSettings())) {
        std::cerr << "Error during exporter initialization.\n";
        return 1;
    }

    if (exporter->Export(scene)) {
        std::cout << "EXPORT OK: Binary FBX created successfully" << std::endl;
    }
    else {
        std::cerr << "ERROR: Export failed!" << std::endl;
    }

    exporter->Destroy();
    manager->Destroy();

    std::cout << "\n============================================" << std::endl;
    std::cout << "SUCCESS! Complete injection + conversion finished." << std::endl;
    std::cout << "Output: BINARY FBX (ready for Blender/RizomUV)" << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}