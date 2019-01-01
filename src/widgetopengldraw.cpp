#include "widgetopengldraw.h"

WidgetOpenGLDraw::WidgetOpenGLDraw(QWidget *parent) : QOpenGLWidget(parent) {
    setMouseTracking(true);
    updateCameraFront();

    std::random_device rd;
    rng = std::mt19937(rd());
}

WidgetOpenGLDraw::~WidgetOpenGLDraw() {
    // Clean state
    gl.glDeleteProgram(programShaderID);
    gl.glDeleteShader(vertexShaderID);
    gl.glDeleteShader(fragmentShaderID);

    for (const auto &object : objects) {
        gl.glDeleteVertexArrays(1, &object.VAO);
        gl.glDeleteBuffers(1, &object.VBO);
        gl.glDeleteBuffers(1, &object.IBO);
        gl.glDeleteBuffers(1, &object.TBO);
    }
}

void WidgetOpenGLDraw::printProgramInfoLog(GLuint obj) {
    int infologLength = 0;
    gl.glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 0) {
        std::unique_ptr<char[]> infoLog(new char[infologLength]);
        int charsWritten = 0;
        gl.glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog.get());
        std::cerr << infoLog.get() << std::endl;
    }
}

void WidgetOpenGLDraw::printShaderInfoLog(GLuint obj) {
    int infologLength = 0;
    gl.glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 0) {
        std::unique_ptr<char[]> infoLog(new char[infologLength]);
        int charsWritten = 0;
        gl.glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog.get());
        std::cerr << infoLog.get() << std::endl;
    }
}

const GLchar* WidgetOpenGLDraw::vertexShaderSource = R"glsl(
    #version 330 core
    layout(location=0) in vec3 position;
    layout(location=1) in vec2 uv;
    layout(location=2) in vec3 normal;

    uniform mat4 P;
    uniform mat4 V;
    uniform mat4 M;

    out vec2 TextureUV;
    out vec3 VertexPos;
    out vec3 NormalInterp;

    mat4 normalMatrix = transpose(inverse(M)); // TODO V or M ??? (Ground works from both sides if V, but it's same on both sides, only distance matters)

    void main() {
        gl_Position = P * V * M * vec4(position, 1.0); // PVM = Final render matrix
        TextureUV = uv;

        vec4 vertPos4 = M * vec4(position, 1.0);
        VertexPos = vec3(vertPos4) / vertPos4.w;
        NormalInterp = vec3(normalMatrix * vec4(normal, 0.0));
    }
)glsl";

const GLchar* WidgetOpenGLDraw::fragmentShaderSource = R"glsl(
    #version 330 core
    // Mesh
    uniform sampler2D TextureUnit;
    // Light
    uniform vec3 LightPos;
    uniform float LightPower;
    uniform vec3 LightColor;
    // Material
    uniform vec3 AmbientColor;
    uniform vec3 DiffuseColor;
    uniform vec3 SpecularColor;
    uniform float SpecularPower; // Shininess factor

    in vec2 TextureUV;
    in vec3 VertexPos;
    in vec3 NormalInterp;

    out vec4 outColor;

    const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space

    // Blinn-Phon shading model, gamma corrected
    vec3 calculateShading() {
        vec3 normal = normalize(NormalInterp);
        vec3 lightDir = LightPos - VertexPos;
        float distance = length(lightDir);
        distance = distance * distance;
        lightDir = normalize(lightDir);

        float lambertian = max(dot(lightDir, normal), 0.0);
        float specular = 0.0;

        if (lambertian > 0.0) {
            vec3 viewDir = normalize(-VertexPos);

            // Blinn-Phong
            vec3 halfDir = normalize(lightDir + viewDir);
            float specAngle = max(dot(halfDir, normal), 0.0);
            specular = pow(specAngle, SpecularPower);
        }

        vec3 colorLinear = AmbientColor +
                           DiffuseColor * lambertian * LightColor * LightPower / distance +
                           SpecularColor * specular * LightColor * LightPower / distance;

        // Apply gamma correction (assume AmbientColor, DiffuseColor and SpecularColor
        // have been linearized, i.e. have no gamma correction in them)
        return pow(colorLinear, vec3(1.0 / screenGamma));
    }

    void main() {
        // Calculate lighting/shading/reflection
        vec3 colorGammaCorrected = calculateShading();

        // Apply texture and use the gamma corrected color in the fragment
        outColor = texture(TextureUnit, TextureUV) * vec4(colorGammaCorrected, 1.0);
    }
)glsl";

void WidgetOpenGLDraw::compileShaders() {
    programShaderID = gl.glCreateProgram();

    // Create and compile the vertex shader
    vertexShaderID = gl.glCreateShader(GL_VERTEX_SHADER);
    std::cout << vertexShaderSource;
    gl.glShaderSource(vertexShaderID, 1, &vertexShaderSource, nullptr);
    gl.glCompileShader(vertexShaderID);
    gl.glAttachShader(programShaderID, vertexShaderID);

    // Create and compile the fragment shader
    fragmentShaderID = gl.glCreateShader(GL_FRAGMENT_SHADER);
    std::cout << fragmentShaderSource;
    gl.glShaderSource(fragmentShaderID, 1, &fragmentShaderSource, nullptr);
    gl.glCompileShader(fragmentShaderID);
    gl.glAttachShader(programShaderID, fragmentShaderID);

    // Link and use created shader program
    gl.glLinkProgram(programShaderID);
    gl.glUseProgram(programShaderID);

    // Print compiled shaders and program
    printShaderInfoLog(vertexShaderID);
    printShaderInfoLog(fragmentShaderID);
    printProgramInfoLog(programShaderID);
}

void WidgetOpenGLDraw::initializeGL() {
    // Load OpenGL functions
    std::cout << "OpenGL context version: " << context()->format().majorVersion() << "." << context()->format().minorVersion() << std::endl;

    if (!gl.initializeOpenGLFunctions()) {
        std::cerr << "Required openGL not supported" << std::endl;
        QApplication::exit(1);
    }

    std::cout << gl.glGetString(GL_VENDOR) << std::endl;
    std::cout << gl.glGetString(GL_VERSION) << std::endl;
    std::cout << gl.glGetString(GL_RENDERER) << std::endl;

    compileShaders();

    // In case we drive more overlapping triangles, we want front to cover the ones in the back
    glEnable(GL_DEPTH_TEST);

    // Draw both faces of triangles
    // glDisable(GL_CULL_FACE);

    // Define data (test objects)
    light = {
        "Light", {0.0f, 2.0f, 0.0f}, 40.0f
    };

    objects = {
        {
            "Ground",
            {
                // Lighting will only work from top (ground doesn't go upside down usually)
                {glm::vec3(-5.0f, 0.0f, -5.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
                {glm::vec3(5.0f,  0.0f, -5.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
                {glm::vec3(5.0f,  0.0f, 5.0f),  glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
                {glm::vec3(-5.0f, 0.0f, 5.0f),  glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)}
            },
            {0, 1, 2, 2, 3, 0}
        }
    };
    applyTextureFromFile("../test/textures/bricks.jpg", &objects.back());

    objects.push_back(makePyramid(3, "Pyramid"));
    objects.back().translation.x = -5.0f;
    objects.back().translation.z = -5.0f;
    applyTextureFromFile("../test/textures/bricks.jpg", &objects.back());

    objects.push_back(makeCube("Cube"));
    objects.back().translation.y += 5.0f;
    applyTextureFromFile("../test/textures/bricks.jpg", &objects.back());

    QStringList paths = {"../test/models/icoSphere.obj"};
    loadModelsFromFile(paths, true);
    objects.back().name = "IcoSphere";
    applyTextureFromFile("../test/textures/steelMesh.jpg", &objects.back());

    // Connect object selection ComboBox and fill it
    QObject::connect(objectSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(selectObject(int)));

    // Add light to object selection dropdown and make it first selected object (same as ComboBox)
    objectSelection->addItem(light.name);
    selectedObject = &light;

    // Buffer data to GPU
    for (auto &object : objects) {
        generateObjectBuffers(object);
    }

    // Set background color
    gl.glClearColor(0.2f, 0.2f, 0.2f, 1);

    const unsigned int err = gl.glGetError();
    if (err != 0) {
        std::cerr << "OpenGL init error: " << err << std::endl;
    }
}

void WidgetOpenGLDraw::generateObjectBuffers(MeshObject &object) {
    // Create Vertex Array Object, carrying properties related with buffer (eg. state of glEnableVertexAttribArray etc.)
    gl.glGenVertexArrays(1, &object.VAO);
    gl.glBindVertexArray(object.VAO);

    // Create and bind Vertex Buffer and load vertices into it
    gl.glGenBuffers(1, &object.VBO);
    gl.glBindBuffer(GL_ARRAY_BUFFER, object.VBO);
    gl.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(object.vertices.size() * sizeof(Vertex)), &object.vertices.front(), GL_STATIC_DRAW);

    // Create and bind Index Buffer and load vertex indices into it
    gl.glGenBuffers(1, &object.IBO);
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.IBO);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(object.indices.size() * sizeof(GLuint)), &object.indices.front(), GL_STATIC_DRAW);

    // Setup vertex attributes (specify layout of vertex data)
    gl.glEnableVertexAttribArray(0);  // We use: layout(location=0) and vec3 position;
    gl.glEnableVertexAttribArray(1);  // We use: layout(location=1) and vec2 uv;
    gl.glEnableVertexAttribArray(2);  // We use: layout(location=2) and vec3 normal;
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, position)));
    gl.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, uv)));
    gl.glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));

#ifdef QT_DEBUG
    // Unbind to avoid accidental modification
    gl.glBindVertexArray(0); // VAO must be first!
    gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif

    // Add to object selection dropdown
    objectSelection->addItem(object.name);
}

void WidgetOpenGLDraw::generateObjectTextureBuffers(MeshObject &object) {
    if (object.textureImage.isNull()) {
        std::cerr << "Generating object texture buffers failed! No image loaded and assigned to object!" << std::endl;
        return;
    }

    // Delete pre-existing Texture Buffer in case it exists
    gl.glDeleteBuffers(1, &object.TBO);

    // Create and bind Texture Buffer and load texture into it (we only support 1 texture unit at this time)
    glGenTextures(1, &object.TBO);
    glBindTexture(GL_TEXTURE_2D, object.TBO);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, object.textureImage.width(), object.textureImage.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, object.textureImage.bits());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Use linear filtering for upscaled textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Use nearest neighbour filtering for downscaled textures
}

void WidgetOpenGLDraw::resizeGL(int w, int h) {
    gl.glViewport(0, 0, w, h);
}

void WidgetOpenGLDraw::paintGL() {
    // Clean color and depth buffer (clean frame start)
    gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Projection matrix
    glm::mat4 P;
    if (projectionOrtho)
        P = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -1000.0f, 1000.0f);
    else
        P = glm::perspective(glm::radians(70.0f), float(width()) / height(), 0.01f, 1000.0f);

    // View matrix (camera position, direction ...)
    glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // Object
    for (const auto &object : objects) {
        if (!object.textureImage.isNull()) {
            // Bind texture to texture units
            gl.glActiveTexture(GL_TEXTURE0); // We only support 1 texture unit at this time
            gl.glBindTexture(GL_TEXTURE_2D, object.TBO);
        } else {
            // Unbind texture specifically to prevent error or already bound texture from being applied
            gl.glBindTexture(GL_TEXTURE_2D, 0);
        }

        gl.glBindVertexArray(object.VAO);

        // Model matrix (object movement)
        glm::mat4 M = glm::mat4(1);
        M = glm::translate(M, object.translation);
        M = glm::rotate(M, object.rotation.x, glm::vec3(1, 0, 0));
        M = glm::rotate(M, object.rotation.y, glm::vec3(0, 0, 1));
        M = glm::rotate(M, object.rotation.z, glm::vec3(0, 1, 0));
        M = glm::scale(M, object.scale);

        // Uniforms
        gl.glUniformMatrix4fv(gl.glGetUniformLocation(programShaderID, "P"), 1, GL_FALSE, glm::value_ptr(P));
        gl.glUniformMatrix4fv(gl.glGetUniformLocation(programShaderID, "V"), 1, GL_FALSE, glm::value_ptr(V));
        gl.glUniformMatrix4fv(gl.glGetUniformLocation(programShaderID, "M"), 1, GL_FALSE, glm::value_ptr(M));
        gl.glUniform1i(gl.glGetUniformLocation(programShaderID, "TextureUnit"), 0);
        gl.glUniform3fv(gl.glGetUniformLocation(programShaderID, "LightPos"), 1, glm::value_ptr(light.translation));
        gl.glUniform1f(gl.glGetUniformLocation(programShaderID, "LightPower"), light.scale.x);
        gl.glUniform3fv(gl.glGetUniformLocation(programShaderID, "LightColor"), 1, glm::value_ptr(light.color));
        gl.glUniform3fv(gl.glGetUniformLocation(programShaderID, "AmbientColor"), 1, glm::value_ptr(object.material.ambientColor));
        gl.glUniform3fv(gl.glGetUniformLocation(programShaderID, "DiffuseColor"), 1, glm::value_ptr(object.material.diffuseColor));
        gl.glUniform3fv(gl.glGetUniformLocation(programShaderID, "SpecularColor"), 1, glm::value_ptr(object.material.specularColor));
        gl.glUniform1f(gl.glGetUniformLocation(programShaderID, "SpecularPower"), object.material.specularPower);

        // Draw
        gl.glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(object.indices.size()), GL_UNSIGNED_INT, nullptr);

#ifdef QT_DEBUG
        // Unbind to avoid accidental modification
        gl.glBindVertexArray(0);
#endif
    }

    const unsigned int err = gl.glGetError();
    if (err != 0) {
        std::cerr << "OpenGL draw error: " << err << std::endl;
    }
}

void WidgetOpenGLDraw::handleKeys(QSet<int> keys, Qt::KeyboardModifiers modifiers) {
    // Camera movement
    if (keys.contains(Qt::Key_W)) {
        // Move camera in the direction its facing
        cameraPos += cameraFront * cameraSpeed;
    }
    if (keys.contains(Qt::Key_S)) {
        // Move camera away from the direction its facing
        cameraPos -= cameraFront * cameraSpeed;
    }
    if (keys.contains(Qt::Key_D)) {
        // Create a vector pointing to the right of the camera (perpendicular to facing direction) and move along it
        // Normalize to retain move speed independent of cameraFront size
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    if (keys.contains(Qt::Key_A)) {
        // Create a vector pointing to the left of the camera (perpendicular to facing direction) and move along it
        // Normalize to retain move speed independent of cameraFront size
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    if (keys.contains(Qt::Key_Q)) {
        // Move camera in the direction of up vector
        cameraPos += cameraUp * cameraSpeed;
    }
    if (keys.contains(Qt::Key_E)) {
        // Move camera in the opposite direction of up vector
        cameraPos -= cameraUp * cameraSpeed;
    }

    // Selected Object movement
    if (keys.contains(Qt::Key_U)) {
        // Move up
        selectedObject->translation.y += 0.25f;
    }
    if (keys.contains(Qt::Key_N)) {
        // Move down
        selectedObject->translation.y -= 0.25f;
    }
    if (keys.contains(Qt::Key_H)) {
        // Move right
        selectedObject->translation.x += 0.25f;
    }
    if (keys.contains(Qt::Key_L)) {
        // Move left
        selectedObject->translation.x -= 0.25f;
    }
    if (keys.contains(Qt::Key_K)) {
        // Move forward
        selectedObject->translation.z += 0.25f;
    }
    if (keys.contains(Qt::Key_J)) {
        // Move backward
        selectedObject->translation.z -= 0.25f;
    }
    if (keys.contains(Qt::Key_Plus)) {
        // Scale up
        selectedObject->scale *= glm::vec3(1.05f);
    }
    if (keys.contains(Qt::Key_Minus)) {
        // Scale down
        selectedObject->scale *= glm::vec3(0.95f);
    }
    if (keys.contains(Qt::Key_X)) {
        // Rotate on X
        int8_t dir = (modifiers.testFlag(Qt::ControlModifier)) ? -1 : 1;
        selectedObject->rotation.x += 0.1f * dir;
    }
    if (keys.contains(Qt::Key_Y)) {
        // Rotate on Y
        int8_t dir = (modifiers.testFlag(Qt::ControlModifier)) ? -1 : 1;
        selectedObject->rotation.z += 0.1f * dir;
    }
    if (keys.contains(Qt::Key_C)) {
        // Rotate on Z
        int8_t dir = (modifiers.testFlag(Qt::ControlModifier)) ? -1 : 1;
        selectedObject->rotation.y += 0.1f * dir;
    }

    // Misc
    if (keys.contains(Qt::Key_P)) {
        // Swap projection (orthogonal or perspective)
        projectionOrtho = !projectionOrtho;
    }

    update(); // Redraw scene
}

void WidgetOpenGLDraw::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        mousePos = event->pos();
    }
}

void WidgetOpenGLDraw::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        QPoint mousePosNew = event->pos();

        cameraYaw -= static_cast<float>(mousePosNew.x() - mousePos.x()) * cameraSensitivity;
        cameraPitch -= static_cast<float>(mousePosNew.y() - mousePos.y()) * cameraSensitivity;

        // Sensible pitch limits
        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        else if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;

        updateCameraFront();

        mousePos = mousePosNew;
    }
}

void WidgetOpenGLDraw::updateCameraFront() {
    double pitchRad = static_cast<double>(glm::radians(cameraPitch));
    double yawRad = static_cast<double>(glm::radians(cameraYaw));

    glm::vec3 front;
    front.x = static_cast<float>(cos(pitchRad) * sin(yawRad));
    front.y = static_cast<float>(sin(pitchRad));
    front.z = static_cast<float>(cos(pitchRad) * cos(yawRad));
    cameraFront = glm::normalize(front);

    update(); // Redraw scene
}

void WidgetOpenGLDraw::selectObject(int index) {
    if (index == 0) {
        selectedObject = &light;
    } else {
        selectedObject = &objects.at(static_cast<uint32_t>(index - 1));
    }
}

bool WidgetOpenGLDraw::isMeshObjectSelected() {
    return selectedObject != &light;
}

void WidgetOpenGLDraw::loadModelsFromFile(QStringList &paths, bool preload) {
    for (auto &path : paths) {
        QFileInfo fileInfo(path);
        MeshObject object(fileInfo.fileName());

        bool loaded = loadModelOBJ(path.toUtf8().constData(), object);
        if (loaded) {
            objects.push_back(object);

            if (!preload) {
                // Buffer new data to GPU
                generateObjectBuffers(objects.back()); // Reference from objects vector, as it is moved in memory when placing into vector!
            }
        }
    }

    if (!preload) {
        // Select last added object
        objectSelection->setCurrentIndex(static_cast<int>(objects.size() - 1));
    }

    update(); // Redraw scene
}

void WidgetOpenGLDraw::applyTextureFromFile(QString path, MeshObject *object) {
    if (!isMeshObjectSelected()) {
        std::cerr << "Texture can only be applied to a mesh object!" << std::endl;
        return;
    }

    if (object == nullptr) {
        object = static_cast<MeshObject *>(selectedObject);
    }

    QImage img;
    if (!img.load(path)) {
        std::cerr << "Texture loading failed!" << std::endl;
        return;
    }

    img = img.convertToFormat(QImage::Format_ARGB32);
    object->textureImage = img;

    // Buffer new data to GPU
    generateObjectTextureBuffers(*object);

    update(); // Redraw scene
}

bool WidgetOpenGLDraw::loadModelOBJ(const char *path, MeshObject &object) {
    std::vector<uint32_t> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> tmpPositions;
    std::vector<glm::vec2> tmpUvs;
    std::vector<glm::vec3> tmpNormals;

    std::ifstream ifs;
    ifs.open(path);

    // Read file
    bool error = false;
    while (!error) {
        std::string lineHeader;
        ifs >> lineHeader;
        if (ifs.eof()) break;

        if (lineHeader == "v") {
            glm::vec3 vertex;
            if (!(ifs >> vertex.x >> vertex.y >> vertex.z)) error = true;
            tmpPositions.push_back(vertex);
        } else if (lineHeader == "vt") {
            glm::vec2 uv;
            if (!(ifs >> uv.x >> uv.y)) error = true;
            tmpUvs.push_back(uv);
        } else if (lineHeader == "vn") {
            glm::vec3 normal;
            if (!(ifs >> normal.x >> normal.y >> normal.z)) error = true;
            tmpNormals.push_back(normal);
        } else if (lineHeader == "f") {
            uint32_t vertexIndex[3], uvIndex[3], normalIndex[3];
            char s; // Indices separated by '/' (slash)
            if (!(ifs >> vertexIndex[0] >> s >> uvIndex[0] >> s >> normalIndex[0]
                      >> vertexIndex[1] >> s >> uvIndex[1] >> s >> normalIndex[1]
                      >> vertexIndex[2] >> s >> uvIndex[2] >> s >> normalIndex[2])) error = true;

            vertexIndices.insert(vertexIndices.end(), {vertexIndex[0], vertexIndex[1], vertexIndex[2]});
            uvIndices.insert(uvIndices.end(), {uvIndex[0], uvIndex[1], uvIndex[2]});
            normalIndices.insert(normalIndices.end(), {normalIndex[0], normalIndex[1], normalIndex[2]});
        } else {
            // Probably a comment (or something else we don't support), eat up the rest of the line
            ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    if (error) {
        std::cerr << "Model OBJ file parsing failed!" << std::endl;
        ifs.close();
        return false;
    }

    // Rearrange data - OBJ indexes all parts separately, OpenGL only supports 1 index buffer
    // Ignore OBJ indexing and just duplicate data for non-index usage (but still directly index them to keep the rest of the code clean)
    // Each triangle
    for (uint32_t v = 0; v < vertexIndices.size(); v += 3) {
        // Each vertex of the triangle
        for (uint32_t i = 0; i < 3; ++i) {
            //  Get indices of wanted parts
            uint32_t vertexIndex = vertexIndices[v + i];
            glm::vec3 position = tmpPositions[vertexIndex - 1];

            uint32_t uvIndex = uvIndices[v + i];
            glm::vec2 uv = tmpUvs[uvIndex - 1];

            uint32_t normalIndex = normalIndices[v + i];
            glm::vec3 normal = tmpNormals[normalIndex - 1];

            // Save parts into Vertex and use current overall index
            object.vertices.push_back({position, uv, normal});
            object.indices.push_back(v + i);
        }
    }

    ifs.close();
    return true;
}

MeshObject WidgetOpenGLDraw::makeCube(QString name) {
    return makeCubeOffset(glm::vec3(0.0f, 0.0f, 0.0f), 0, name);
}

MeshObject WidgetOpenGLDraw::makeCubeOffset(glm::vec3 baseVertex, GLuint baseIndex, QString name) {
    std::uniform_real_distribution<float> dist(0, 1);

    MeshObject cube = {
        name,
        // Some vertices duplicated to fit indexing of UVs
        {
            {baseVertex + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.66f),  glm::vec3(-1.0f, 2.0f, -1.0f)},
            {baseVertex + glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.25f, 0.66f), glm::vec3(-1.0f, -1.0f, -1.0f)},
            {baseVertex + glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.33f),  glm::vec3(2.0f, 2.0f, -1.0f)},
            {baseVertex + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.25f, 0.33f), glm::vec3(2.0f, -1.0f, -1.0f)},

            {baseVertex + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.5f, 0.66f),  glm::vec3(-1.0f, -1.0f, 2.0f)},
            {baseVertex + glm::vec3(1.0f, 0.0f, 1.0f), glm::vec2(0.5f, 0.33f),  glm::vec3(2.0f, -1.0f, 2.0f)},
            {baseVertex + glm::vec3(0.0f, 1.0f, 1.0f), glm::vec2(0.75f, 0.66f), glm::vec3(-1.0f, 2.0f, -1.0f)},
            {baseVertex + glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.75f, 0.33f), glm::vec3(2.0f, 2.0f, 2.0f)},

            {baseVertex + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.66f),  glm::vec3(-1.0f, 2.0f, -1.0f)},
            {baseVertex + glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.33f),  glm::vec3(2.0f, 2.0f, -1.0f)},

            {baseVertex + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.25f, 1.0f),  glm::vec3(-1.0f, 2.0f, -1.0f)},
            {baseVertex + glm::vec3(0.0f, 1.0f, 1.0f), glm::vec2(0.5f, 1.0f),   glm::vec3(-1.0f, 2.0f, 2.0f)},

            {baseVertex + glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(0.25f, 0.0f),  glm::vec3(2.0f, 2.0f, -1.0f)},
            {baseVertex + glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.5f, 0.0f),   glm::vec3(2.0f, 2.0f, 2.0f)},
        },
        {
            baseIndex + 0, baseIndex + 2, baseIndex + 1,baseIndex + 1, baseIndex + 2, baseIndex + 3, // Front
            baseIndex + 4, baseIndex + 5, baseIndex + 6, baseIndex + 5, baseIndex + 7, baseIndex + 6, // Back
            baseIndex + 6, baseIndex + 7, baseIndex + 8, baseIndex + 7, baseIndex + 9, baseIndex + 8, // Top
            baseIndex + 1, baseIndex + 3, baseIndex + 4, baseIndex + 3, baseIndex + 5, baseIndex + 4, // Bottom
            baseIndex + 1, baseIndex + 11, baseIndex + 10, baseIndex + 1, baseIndex + 4, baseIndex + 11, // Left
            baseIndex + 3, baseIndex + 12, baseIndex + 5, baseIndex + 5, baseIndex + 12, baseIndex + 13 // Right
        }
    };

    return cube;
}

MeshObject WidgetOpenGLDraw::makePyramid(uint32_t rows, QString name) {
    MeshObject pyramid(name);

    float offset = 0.0f;
    for (uint32_t row = 0; row < rows; ++row) {
        for (uint32_t i = 0; i < rows - row; ++i) {
            for (uint32_t j = 0; j < rows - row; ++j) {
                // Make cube and merge it into the pyramid
                MeshObject cube = makeCubeOffset(glm::vec3(offset + i, row, offset + j), static_cast<GLuint>(pyramid.vertices.size()));
                pyramid.vertices.insert(std::end(pyramid.vertices), std::begin(cube.vertices), std::end(cube.vertices));
                pyramid.indices.insert(std::end(pyramid.indices), std::begin(cube.indices), std::end(cube.indices));
            }
        }

        offset += 0.5f;
    }

    return pyramid;
}
