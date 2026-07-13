#include "SceneLoader.hpp"

#include "util/umath.hpp"
#include "util/optional.hpp"

#include <cstring>
#include <iostream>

using std::string;
using std::unique_ptr, std::shared_ptr;
using std::make_shared, std::make_unique;
using std::cerr;

using std::pair;
using std::ifstream;

/*
 * TODO: documentation
 */

const bool VERBOSE = false;

std::shared_ptr<Shape> shapeFromString(const std::string& type);

void SceneLoader::loadSceneFile(std::unique_ptr<Camera>& cam, std::unique_ptr<Scene>& scene) 
{
    if (scene == nullptr || cam == nullptr) {
        throw std::logic_error("loadSceneFile: One or more arguments are null pointers");
    }
    if (location.empty()) { 
        std::cerr << "No input file given\n"; 
        return; 
    }

    sceneDir = srcDir+"scenes/";
    textureDir = srcDir+"textures/";
    modelDir = srcDir+"models/";

    file.open(location);
    if (!file.is_open()) file.open(sceneDir+location);

    if (!file.is_open()) {
        std::cerr << "Couldn't open file " << location << '\n'
                  << "File not found in data directory or model subdirectory.\n";
        return;
    } else {
        // not great, this is a hacky way to access the entire
        // json data as a string, which is not great for large files
        jsonData = (std::stringstream() << file.rdbuf()).str();
    }
    file.close();

    const uint tok_size = 2048U;

    jsmn_parser parser;
    jsmntok_t tokens[tok_size];
    jsmn_init(&parser);

    // r is either an error code or the number of actual tokens used
    int r = jsmn_parse(&parser, jsonData.c_str(), jsonData.length(), tokens, tok_size);

    /*
    JSMN_ERROR_INVAL - bad token, JSON string is corrupted
    JSMN_ERROR_NOMEM - not enough tokens, JSON string is too large
    JSMN_ERROR_PART - JSON string is too short, expecting more JSON data
    */
    if (r < 0) {
        std::cerr << "Failed to parse JSON: " << r << '\n';
        switch(r) {
        case JSMN_ERROR_INVAL:
            std::cerr << "corrupted string\n";
            break;
        case JSMN_ERROR_NOMEM:
            std::cerr << "string is too large\n";
            break;
        case JSMN_ERROR_PART:
            std::cerr << "expected more data\n";
            break;
        default:
            // gcc might complain w/o this
            break;
        }
        return;
    }

    // Assume the top-level element is an object
    if (r < 1 || tokens[0].type != JSMN_OBJECT) {
        std::cerr << "Object expected\n";
        return;
    }

    int ignored_tokens = 0;

    // All tokens are parsed in order
    for (int i = 1, prev_i = 1; i < r; prev_i = i) {
        if (jsonstreq(&tokens[i], "camera")) {
            // i now refers to the object-value of the "camera" key
            if (tokens[++i].type != JSMN_OBJECT) continue;
            i += parseCameraProperties(&tokens[i], cam);
            
        } else if (jsonstreq(&tokens[i], "materials")) {
            if (tokens[++i].type != JSMN_OBJECT) continue;
            i += parseMaterials(&tokens[i], scene);

        } else if (jsonstreq(&tokens[i], "lights")) {
            if (tokens[++i].type != JSMN_ARRAY) continue;
            i += parseLights(&tokens[i], scene);
            
        } else if (jsonstreq(&tokens[i], "shapes")) {
            if (tokens[++i].type != JSMN_ARRAY) continue;
            i += parseShapes(&tokens[i], scene);

        } else {
            if (VERBOSE)
                printf("\"Unrecognized token\": %.*s\n", 
                    tokens[i].end - tokens[i].start, 
                    jsonData.c_str() + tokens[i].start);
            ++ignored_tokens;
            ++i;
        }

        // i should be incremented in all branches
        if (prev_i >= i) {
            throw std::logic_error("Infinite loop detected.");
        }
    }

    if (ignored_tokens > 0) {
        std::clog << ignored_tokens << " were unused.\n";
    }
}

// Section parsing
// these functions return the offset from the provided token to the next unnested token

// repeating logic here; could create new classes to abstract away the details
// of reading object and array tokens

int SceneLoader::parseCameraProperties(const jsmntok_t* obj_tok, std::unique_ptr<Camera>& cam)
{
    int j = 1; // the offset to the next key token
    // this pointer arithmetic is not very safe
    for (int prop = 0; prop < obj_tok->size; ++prop) {
        auto key = obj_tok+j, value = key+1;
        if (jsonstreq(key, "distance")) {
            double initDist = doubleFromToken(value);
            cam->setInitDistance(initDist);  
        } else if (jsonstreq(key, "antialias")) {
            int samples = intFromToken(value);
            samples = samples < 1 ? 1 : samples;
            cam->setAntialiasSamples((uint)samples);
        } else if (jsonstreq(key, "ambientSamples")) {
            int samples = intFromToken(value);
            samples = samples < 0 ? 0 : samples;
            cam->setAmbientOcclusionSamples((uint)samples);
        } else if (jsonstreq(key, "ambientColor")) {
            vec3 clr = float3FromToken(value);
            cam->setGlobalAmbientColor(clr);  
        } else if (jsonstreq(key, "occlusionRadius")) {
            double occlRadius = doubleFromToken(value);
            cam->setAmbientOccludingRadius(occlRadius);
        } else if (jsonstreq(key, "focusLength")) {
            float length = doubleFromToken(value);
            cam->setFocusLength(length);
        } else if (jsonstreq(key, "focalRadius")) {
            float r = doubleFromToken(value);
            cam->setFocalRadius(r);
        } else if (jsonstreq(key, "fov")) {
            float fov = doubleFromToken(value);
            cam->setFOV(fov);
        } else if (jsonstreq(key, "sky")) {
            if (jsonstreq(value, "haze")) {
                cam->setSky(Camera::SkyType::Haze);
            } else if (jsonstreq(value, "ambient")) {
                cam->setSky(Camera::SkyType::Ambient);
            } else if (jsonstreq(value, "void")) {
                 cam->setSky(Camera::SkyType::Void);
            } else if (value->type == JSMN_STRING) {
                string textureFilePath = textureDir + stringFromToken(value);
                cam->setSky(Camera::SkyType::SphereMap);
                // https://stackoverflow.com/questions/12774207/
                ifstream f(textureFilePath.c_str());
                if (!f.good()) {
                    std::cerr << "Unknown sky type: " 
                              << print_token(value) << '\n'; 
                } else {
                    cam->setSkyTexture(std::move(
                        make_unique<ImageTexture>(textureFilePath)
                    ));
                }
            }
        } else if (jsonstreq(key, "rotation")) {
            // [yaw, pitch, roll]
            vec3 rot = float3FromToken(value);
            cam->setRotation(rot);
        } else if (jsonstreq(key, "from")) {
            vec3 eye = float3FromToken(value);
            cam->setCameraPos(eye);
        } else if (jsonstreq(key, "to")) {
            vec3 center = float3FromToken(value);
            cam->setLookAtPos(center);
        } else if (jsonstreq(key, "up")) {
            vec3 up = float3FromToken(value);
            cam->setUpVector(up);
        }

        // if the value-token is not a primitive,
        // increment the current token pointer past the object's
        // or array's tokens
        j += 1 + offsetToNextKey(value);
    }
    // offset to next object (camera, materials, etc.)
    return j;
}

int SceneLoader::parseLights(const jsmntok_t* arr_tok, std::unique_ptr<Scene>& scene)
{
    int j = 1;
    for (int item = 0; item < arr_tok->size; ++item) {
        // skip any items that do not contain light properties
        const jsmntok_t* l_tok = arr_tok+j; // this should be an object token
        if (l_tok->type != JSMN_OBJECT) {
            j += offsetToNextKey(l_tok);
            continue;
        }

        std::shared_ptr<Light> light = std::make_shared<Light>();
        int k = 1; // start after the object token
        for (int prop = 0; prop < (arr_tok+j)->size; ++prop) {
            const jsmntok_t *key = l_tok+k, *value = l_tok+k+1;
            if (jsonstreq(key, "position") && value->type == JSMN_ARRAY) {
                light->pos = float3FromToken(value);
            } else if (jsonstreq(key, "intensity") && value->type == JSMN_PRIMITIVE) {
                light->intensity = doubleFromToken(value);
            } else if (jsonstreq(key, "radius") && value->type == JSMN_PRIMITIVE) {
                light->setRadius(doubleFromToken(value));
            } else if (jsonstreq(key, "samples") && value->type == JSMN_PRIMITIVE) {
                light->setSamples(intFromToken(value));
            } else {
                std::cerr << "invalid property when parsing lights: " << key << '\n';
            }
            // Add another 1 to account for the key
            k += 1 + offsetToNextKey(value);
        }
        j += k;    
        scene->pushLight(light);
    }



    return j;
}

int SceneLoader::parseMaterials(const jsmntok_t* obj_tok, std::unique_ptr<Scene>& scene) 
{
    assert(obj_tok->type == JSMN_OBJECT);

    int j = 1;
    for (int item = 0; item < obj_tok->size; ++item) {
        const jsmntok_t* mat_name_tok = obj_tok + j;
        const jsmntok_t* mat_prop_tok = mat_name_tok + 1;

        if (mat_name_tok->type != JSMN_STRING) {
            j += 1 + offsetToNextKey(mat_prop_tok);
            continue;
        }

        std::shared_ptr<Material> material = std::make_shared<Material>();
        bool fresnelExplicitSet = false;

        int prop_ind = 1;
        for (int prop = 0; prop < mat_prop_tok->size; ++prop) {
            auto key = mat_prop_tok+prop_ind, value = key + 1;

            // alternate names for the same property
            auto isReflectProp = [&key, this]() {
                return jsonstreq(key, "reflCoeff") ||
                       jsonstreq(key, "reflectance") ||
                       jsonstreq(key, "reflective");
            };

            auto isRefractProp = [&key, this]() {
                return jsonstreq(key, "refrIndex") ||
                       jsonstreq(key, "refractionIndex");
            };

            if (jsonstreq(key, "exponent")) {
                material->exponent = doubleFromToken(value);
            } else if (isReflectProp()) {
                material->reflCoeff = doubleFromToken(value);
                if (!fresnelExplicitSet) material->fresnelCoeff = 1.f;
            } else if (isRefractProp()) {
                material->refrIndex = doubleFromToken(value);
            } else if (jsonstreq(key, "transparency")) {
                material->transparency = doubleFromToken(value);
            } else if (jsonstreq(key, "fresnelCoeff")) {
                material->fresnelCoeff = doubleFromToken(value);  
                fresnelExplicitSet = true;  
            } else if (jsonstreq(key, "fuzz")) {
                material->fuzz = doubleFromToken(value);
            } else if (jsonstreq(key, "reflectionSamples")) {
                int reflSamples = intFromToken(value);
                if (reflSamples < 1) {
                    std::cerr << "Invalid reflectionSamples value for"
                              << print_token(mat_name_tok) << ": "
                              << print_token(value) << '\n';
                } else material->reflSamples = reflSamples;
                
            } else {
                pair<bool, int> rval = tryLoadMaterialComps(material, key, value);
                bool success = rval.first; int incr = rval.second;
                if (!success) {
                    std::cerr << "invalid property : " << print_token(key) << '\n';
                }
                prop_ind += incr;
                // already at next key if the value is object (and thus incr > 0)
                if (incr > 0) continue; 
            } 
            // else if (!tryLoadMaterialComps(material, key, value)) {
            //     std::cerr << "invalid property : " << print_token(key) << '\n';
            // };
            
            prop_ind += 1 + offsetToNextKey(value); 
        }
        scene->writeMaterial(stringFromToken(mat_name_tok), material);
        j += prop_ind + 1; // add 1 to account for mat name 
    }
    return j;
}

int SceneLoader::parseShapes(const jsmntok_t* arr_tok, std::unique_ptr<Scene>& scene) 
{
    int j = 1;
    for (int item = 0; item < arr_tok->size; ++item) {
        const jsmntok_t* s_tok = arr_tok+j; // should be an object token
        if (s_tok->type != JSMN_OBJECT) {
            j += s_tok->size;
            continue;
        }

        std::shared_ptr<Shape> shape;

        int prop_ind = parseShape(s_tok, scene, shape);
        scene->pushShape(shape);

        j += prop_ind;
    }
    return j;
}

// For use in recursively parsing shapes
int SceneLoader::parseShape(
    const jsmntok_t* s_tok, 
    unique_ptr<Scene>& scene, 
    shared_ptr<Shape>& shapeToInit) 
{
    if (s_tok->type != JSMN_OBJECT) {
        std::cerr << "parseShape(): token is of type "
                  << s_tok->type << '\n'; 
        return 0;
    }

    ShapeProperties prop(*this);
    std::shared_ptr<Shape> &shape = shapeToInit;

    int prop_ind = 1;
    for (int p = 0; p < s_tok->size; ++p) {
        auto key = s_tok + prop_ind, value = key+1;

        if (jsonstreq(key, "shape")) {
            prop.type = shapeTypeFromToken(value);
            shape = createShape(prop.type);
        } 
        else if (jsonstreq(key, "position")) {
            prop.pos = float3FromToken(value);
        } else if (jsonstreq(key, "scale")) {
            prop.scl = float3FromToken(value);
        } else if (jsonstreq(key, "rotation")) {
            prop.rot = float3FromToken(value);
            prop.rot.x = glm::radians(prop.rot.x);
            prop.rot.y = glm::radians(prop.rot.y);
            prop.rot.z = glm::radians(prop.rot.z);
        } 
        else if (jsonstreq(key, "nextPosition")) {
            prop.npos = float3FromToken(value);
            prop.movePos = true;
        } else if (jsonstreq(key, "nextScale")) {
            prop.nscl = float3FromToken(value);
            prop.moveScl = true;
        } 
        // else if (jsonstreq(key, "nextRotation")) {
        //     // prop.rot = float3FromToken(value);
        //     // prop.rot.x = glm::radians(prop.rot.x);
        //     // prop.rot.y = glm::radians(prop.rot.y);
        //     // prop.rot.z = glm::radians(prop.rot.z);
        
        // } 
        else if (jsonstreq(key, "material")) {
            prop.smat = scene->getMaterial(stringFromToken(value));
        } 
        else if (jsonstreq(key, "file")) {
            prop.mesh_filename = stringFromToken(value);
        } else if (jsonstreq(key, "left")) {
            // Parsing nested shapes requires accounting for 
            // nested tokens within nested tokens
            prop_ind += 1 + parseShape(value, scene, prop.left);
            continue;
        } else if (jsonstreq(key, "right")) {
            prop_ind += 1 + parseShape(value, scene, prop.right);
            continue;
        } else if (jsonstreq(key, "operator")) {
            string operationType = stringFromToken(value);
            if (operationType == "intersection") {
                prop.operationType = OperationType::Intersection;
            } else if (operationType == "union" ) {
                prop.operationType = OperationType::Union;
            } else if (operationType == "difference") {
                prop.operationType = OperationType::Difference;
            } else {
                cerr << "operationType " << operationType 
                     << " is not a valid operation type. Valid types: "
                     << "intersection, union, difference\n";
                prop.operationType = OperationType::None;
            }

        }
        else {
            std::cerr << "parseShapes: invalid property: "
                        << print_token(key) << '\n';
        }

        prop_ind += 1 + offsetToNextKey(value);
    }

    if (shape != nullptr) {
        prop.applyProperties(shape);
    } else {
        std::cerr << "Shape not created.\n";
    }

    return prop_ind;
}

// Misc. helper functions

SceneLoader::PRIM SceneLoader::typeOfPrimitiveAt(int offset) 
{
    // to be determined
    return PRIM::NONE;
}

bool SceneLoader::charIsNumeric(int offset)
{
    // Relies on ASCII orderings of characters
    if (jsonData.at(offset) == '-') return true;
    for (char c = '0'; c <= '9'; c++) {
        if (jsonData.at(offset) == c) return true;
    }
    return false;
}

bool SceneLoader::jsonstreq(const jsmntok_t* tok, const std::string& str) 
{
    size_t tok_len = tok->end - tok->start;
    if (tok->type == JSMN_STRING && str.length() == tok_len && 
        strncmp(&jsonData.at(tok->start), str.c_str(), tok_len) == 0) {
        return true;
    }
    return false;
}

std::shared_ptr<Shape> SceneLoader::createShape(SHAPE_TYPE type) 
{
    switch(type) {
    case SHAPE_TYPE::sphere:
        return make_shared<Sphere>(); 
    case SHAPE_TYPE::plane:
        return make_shared<Plane>();
    case SHAPE_TYPE::mesh:
        return make_shared<Mesh>();
    case SHAPE_TYPE::box:
        return make_shared<Box>();
    case SHAPE_TYPE::cylinder: 
        return make_shared<Cylinder>();
    case SHAPE_TYPE::csg:
        return make_shared<CSG>();
    case SHAPE_TYPE::torus:
        return make_shared<Torus>();
    default:
        std::cerr << "createShape: unrecognized shape type\n";
        return nullptr;
    }
}

void SceneLoader::ShapeProperties::applyProperties(shared_ptr<Shape>& shape)
{
    ModelMatConstr modelMat;
    modelMat.setPosition(pos);
    modelMat.setRotation(rot);
    modelMat.setScale(scl);
    mat4 initialModel = modelMat.getMatrix();

    // Not a good look
    if (movePos || moveScl || moveRot) { 
        if (!movePos) npos = pos;
        if (!moveScl) nscl = scl;
        if (!moveRot) nrot = rot;
        shape->setNextModelTransforms(npos, nrot, nscl); 
    }
    
    // Hack to initialize a mesh object
    // bool isMesh = dynamic_cast<Mesh*>(shape.get()) != nullptr;

    // safer
    switch(this->type) {
    case SHAPE_TYPE::mesh:
        // in modelDir, subfolder of srcDir?
        if (ifstream(loader.modelDir+mesh_filename).good()) 
            shape = make_shared<Mesh>(mesh_filename, loader.modelDir);
        // in srcDir?
        else if (ifstream(loader.srcDir+mesh_filename).good()) 
            shape = make_shared<Mesh>(mesh_filename, loader.srcDir);

        else std::cerr << "applyProperties: " << mesh_filename << " not found in "
                       << loader.sceneDir << " or " << loader.modelDir << '\n';
        break;
    case SHAPE_TYPE::csg:
        if (left == nullptr || right == nullptr) {
            std::cerr << "applyProperties: left shape or right shape undeclared\n";
            break;
        } else if (operationType == OperationType::None) {
            std::cerr << "applyProperties: operationType undeclared\n";
            break;
        }
        shape = make_shared<CSG>(operationType, left, right);
        break;
    default:
        break;
    }

    shape->setModelMatrix(initialModel);
    shape->setMaterial(smat);
};

// bool indicates success status, int is increment needed to next key - 1,
// which is > 0 only for objects
pair<bool, int> SceneLoader::tryLoadMaterialComps(
    shared_ptr<Material>& material, 
    const jsmntok_t *key, 
    const jsmntok_t *value) 
{
    bool isFilename = value->type == JSMN_STRING;
    bool isFloat3 = value->type == JSMN_ARRAY && 
                    charIsNumeric((value+1)->start);
    bool isObj = value->type == JSMN_OBJECT;

    string textureFilePath;
    int nextOffset = 0; // increment by non-zero only if value is an object

    if (value->type == JSMN_UNDEFINED) return {false, 0};
    else if (!isFilename && !isFloat3 && !isObj) {
        std::cerr << "invalid value type; must be a string, numeric array, or object: "
                  << print_token(value) << '\n';
        return {false, 0};
    }
    else if (isFilename) textureFilePath = textureDir+stringFromToken(value);

    auto f = ifstream(textureFilePath);
    bool fileLoaded = f.good();
    if (isFilename && !fileLoaded) {
        std::cerr << "Failed to load file: " << textureFilePath << '\n';
        return {false, 0};
    }

    auto changeMat = [&](shared_ptr<Texture>& comp) {
        if (isFloat3) comp->init(float3FromToken(value));
        else if (isFilename) comp = make_shared<ImageTexture>(textureFilePath);
        else if (isObj) nextOffset = 1 + parseTexture(value, comp);
    };

    // unorthodox formatting: the joy of owning this code alone
    if (jsonstreq(key, "ambient")) { changeMat(material->ambient); } else 
    if (jsonstreq(key, "diffuse")) { changeMat(material->diffuse); } else 
    if (jsonstreq(key, "specular")) { changeMat(material->specular); } else 
    if (jsonstreq(key, "emissive")) { changeMat(material->emissive); } else 
    if (jsonstreq(key, "absorb")) 
    {
        if (isFloat3) { 
            // Leave as-is unless the highest allowed clr changes
            vec3 clr = float3FromToken(value);
            material->absorb->init(clr); 
        }
        else if (isFilename) {
            // absorbCoeff would be calculated from the highest clr component
            material->absorb = make_shared<ImageTexture>(textureFilePath);
        }    
    }

    return {true, nextOffset};
}

/*
\texture\ definition

{
	"diffuse" : {
		"file" : "filepath", 
		"color" : [0,0,0], 
		"alpha" : 0, 
	}, // file with color
	
	"ambient" : {
		"even" : {},
		"odd" : {}
	}, // checker texture
	"specular" : [0,0,0], // plain color
	"emissive" : "filename.png" // plain file
}
*/

struct TextureProperties {
    optional<string> filename;
    optional<vec3> color;
    optional<float> alpha;

    optional<shared_ptr<Texture>> even;
    optional<shared_ptr<Texture>> odd;

    constexpr bool isImageTexture() { return filename.has_value(); };
    constexpr bool isCheckerTexture() { return even.has_value() || odd.has_value(); };
};

// Assume that t_tok is a value from a key-value pair 
int SceneLoader::parseTexture(const jsmntok_t* t_tok, shared_ptr<Texture>& text) {
    // the isFilePath and isVec checks are for recursive calls
    bool isFilePath = t_tok->type == JSMN_STRING;
    bool isVec = t_tok->type == JSMN_ARRAY && 
                 charIsNumeric((t_tok+1)->start);
    bool isObj = t_tok->type == JSMN_OBJECT;
    
    if (isFilePath) { 
        // using an image for the texture, plain as it is
        const string textureFilePath = textureDir+stringFromToken(t_tok);
        text = make_shared<ImageTexture>(textureFilePath);
        // Return just offsetToNextKey() so that the upper level 
        // function has to +1 to match convention
        return offsetToNextKey(t_tok); 
    } else if (isVec) {
        const vec3 clr = float3FromToken(t_tok);
        text = make_shared<ColorTexture>(clr);
        return offsetToNextKey(t_tok);
    } else if (!isObj) {
        std::cerr << "invalid value type; must be a string, numeric array, or object: "
                  << print_token(t_tok) << '\n';
        return offsetToNextKey(t_tok);
    }

    int prop_ind = 1;
    TextureProperties tp;
    
    // object parsing to get various properties of a texture;
    // the presence of certain initialized properties are used
    // to determine what texture to create, but an invalid combination
    // outputs an error instead (default to set color or black)
    for (int p = 0; p < t_tok->size; ++p) {
        const jsmntok_t *key = t_tok + prop_ind, *value = key + 1;
        
        if (jsonstreq(key, "file")) {
            tp.filename = stringFromToken(value);
        } else if (jsonstreq(key, "color")) {
            tp.color = float3FromToken(value);
        } else if (jsonstreq(key, "alpha")) {
            tp.alpha = doubleFromToken(value);
        } else if (jsonstreq(key, "even")) {
            shared_ptr<Texture> etext;
            prop_ind += 1 + parseTexture(value, etext);
            tp.even = etext;
            continue; // continue since alr increment  prop_ind
        } else if (jsonstreq(key, "odd")) {
            shared_ptr<Texture> otext;
            prop_ind += 1 + parseTexture(value, otext);
            tp.odd = otext;
            continue;
        } else {
            std::cerr << "Unknown key: " << print_token(key) << '\n';
        }

        prop_ind += 1 + offsetToNextKey(value);
    }

    if (tp.isImageTexture()) {
        const string textureFilePath = textureDir + tp.filename.value();
        auto imgText = make_shared<ImageTexture>(textureFilePath);
        imgText->init(tp.color.value_or(vec3(0.f)));
        imgText->setAlpha(tp.alpha.value_or(0.f));
        text = imgText;
    } else if (tp.isCheckerTexture()) {
        auto checkText = make_shared<CheckerTexture>();
        if (tp.even.has_value()) checkText->setEven(tp.even.value());
        if (tp.odd.has_value()) checkText->setOdd(tp.odd.value());
        text = checkText;
    } else if (tp.color.has_value()) {
        auto clrText = make_shared<ColorTexture>(tp.color.value());
        text = clrText;
    } else {
        auto defaultCheckText = make_shared<GradientTexture>();
        text = defaultCheckText;
    }

    return prop_ind;
}

// Token conversion

glm::vec3 SceneLoader::float3FromToken(const jsmntok_t* tok)
{
    if (tok->type != JSMN_ARRAY) {
        std::cerr << "float3FromToken: token is not an array - "
                    << tok->type << '\n';
        return glm::vec3(0.0f);
    } else if (tok->size < 3) {
        std::cerr << "float3FromToken: Size of array must be >= 3\n";
        return glm::vec3(0.0f);
    };
    glm::vec3 v;
    v.x = (float) doubleFromToken(tok+1);
    v.y = (float) doubleFromToken(tok+2);
    v.z = (float) doubleFromToken(tok+3);
    return v;
}

double SceneLoader::doubleFromToken(const jsmntok_t* tok)
 {
    if (tok->type == JSMN_PRIMITIVE && charIsNumeric(tok->start)) {
        return std::strtod(&jsonData.at(tok->start), nullptr);
    }
    return 0.0; // bogus
}

int SceneLoader::intFromToken(const jsmntok_t* tok)
{
    if (tok->type == JSMN_PRIMITIVE && charIsNumeric(tok->start)) {
        return (int) std::strtol(&jsonData.at(tok->start), nullptr, 10);
    }
    return 0;
}

bool SceneLoader::boolFromToken(const jsmntok_t* tok)
{
    if (tok->type == JSMN_PRIMITIVE) {
        if (jsonData.at(tok->start) == 't') return true;
        if (jsonData.at(tok->start) == 'f') return false;
    }
    std::cerr << "boolFromToken: unrecognized value, returning false";
    return false;
}

std::string SceneLoader::stringFromToken(const jsmntok_t* tok)
{
    if (tok->type != JSMN_STRING) {
        std::cerr << "stringFromToken: expected token of type JSMN_STRING = 4, got"
                  << tok->type << "instead.\n";
        throw std::runtime_error("");
    }
    return print_token(tok);
}

SceneLoader::SHAPE_TYPE SceneLoader::shapeTypeFromToken(const jsmntok_t* tok) 
{
    string type = stringFromToken(tok);
    if (type == "sphere" || type == "ball") {
        return SHAPE_TYPE::sphere; 
    } else if (type == "plane") {
        return SHAPE_TYPE::plane;
    } else if (type == "mesh") { 
        return SHAPE_TYPE::mesh;
    } else if (type == "box") {
        return SHAPE_TYPE::box;
    } else if (type == "cylinder") {
        return SHAPE_TYPE::cylinder;
    } else if (type == "csg") {
        return SHAPE_TYPE::csg;  
    } else if (type == "torus") {
        return SHAPE_TYPE::torus;
    } else {
        std::cerr << "shapeTypeFromToken: unrecognized shape type:"
                  << type << '\n';
        return SHAPE_TYPE::none;
    }
}