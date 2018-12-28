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

    gl.glDeleteBuffers(1, &vertexBufferID);
    gl.glDeleteBuffers(1, &indexBufferID);

    for (const auto &object : objects) {
        gl.glDeleteVertexArrays(1, &object.vertexArrayObject);
    }
}

void WidgetOpenGLDraw::printProgramInfoLog(GLuint obj) {
    int infologLength = 0;
    gl.glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 0) {
        std::unique_ptr<char[]> infoLog(new char[infologLength]);
        int charsWritten = 0;
        gl.glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog.get());
        std::cerr << infoLog.get() << "\n";
    }
}

void WidgetOpenGLDraw::printShaderInfoLog(GLuint obj) {
    int infologLength = 0;
    gl.glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 0) {
        std::unique_ptr<char[]> infoLog(new char[infologLength]);
        int charsWritten = 0;
        gl.glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog.get());
        std::cerr << infoLog.get() << "\n";
    }
}

const GLchar* WidgetOpenGLDraw::vertexShaderSource = R"glsl(
    #version 330 core
    layout(location=0) in vec3 position;
    layout(location=1) in vec4 color;
    uniform mat4 PVM;
    out vec4 Color;
    void main() {
        gl_Position = PVM * vec4(position, 1);
        Color = color;
    }
)glsl";

const GLchar* WidgetOpenGLDraw::fragmentShaderSource = R"glsl(
    #version 330 core
    in vec4 Color;
    out vec4 outColor;
    void main() {
        outColor = Color;
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
    std::cout << "OpenGL context version: "<< context()->format().majorVersion() <<"." <<context()->format().minorVersion()<<std::endl;

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

    // Define data
    objects = {
        // Ground
        {
            "Ground",
            {
                {glm::vec3(-5, 0, -5), glm::vec4(0.7f, 0, 0, 1)},
                {glm::vec3(5, 0, -5),  glm::vec4(0.7f, 0, 0, 1)},
                {glm::vec3(5, 0, 5),   glm::vec4(0.7f, 0, 0, 1)},
                {glm::vec3(-5, 0, 5),  glm::vec4(0.7f, 0, 0, 1)}
            },
            {0, 1, 2, 2, 3, 0}
        }
    };

    objects.push_back(makePyramid(glm::vec3(-1, 0, -1), 3, "Pyramid 1"));

    // Connect object selection ComboBox and fill it
    QObject::connect(objectSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(setObject(int)));
    selectedObject = &objects.front(); // First selected object same as first selected ComboBox item

    // Generate buffers
    gl.glGenBuffers(1, &vertexBufferID); // Create Vertex Buffer
    gl.glGenBuffers(1, &indexBufferID); // Create Index (Element) Buffer

    // Buffer data
    updateBuffers();
    for (auto &object : objects) {
        generateObjectVAO(&object);
    }

    // Set background color
    gl.glClearColor(0.2f, 0.2f, 0.2f, 1);

    const unsigned int err = gl.glGetError();
    if (err != 0) {
        std::cerr << "OpenGL init error: " << err << std::endl;
    }
}

void WidgetOpenGLDraw::updateBuffers() {
    // Buffer data to GPU
    // Compile full object sizes and their offsets inside the resulting buffer
    size_t verticesSize = 0;
    size_t indicesSize = 0;
    GLuint vertexOffset = 0;
    GLuint indexOffset = 0;
    for (auto &object : objects) {
        object.vertexBufferOffset = vertexOffset;
        verticesSize += object.vertices.size();
        vertexOffset += object.vertexBufferSize();

        object.indexBufferOffset = indexOffset;
        indicesSize += object.indices.size();
        indexOffset += object.indexBufferSize();
    }

    // Bind Vertex Buffer and load vertices into it
    gl.glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    gl.glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verticesSize * sizeof(Vertex)), nullptr, GL_STATIC_DRAW);

    for (const auto &object : objects) {
        gl.glBufferSubData(GL_ARRAY_BUFFER, object.vertexBufferOffset, object.vertexBufferSize(), &object.vertices.front());
    }

    // Bind Index (Element) Buffer and load vertex indices into it
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indicesSize * sizeof(GLuint)), nullptr, GL_STATIC_DRAW);

    for (const auto &object : objects) {
        gl.glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, object.indexBufferOffset, object.indexBufferSize(), &object.indices.front());
    }
}

void WidgetOpenGLDraw::generateObjectVAO(Object *object) {
    // Create Vertex Array Object for given object, carrying properties related with buffer (eg. state of glEnableVertexAttribArray etc.)
    gl.glGenVertexArrays(1, &object->vertexArrayObject);
    gl.glBindVertexArray(object->vertexArrayObject);

    gl.glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

    // Specify layout of vertex data
    gl.glEnableVertexAttribArray(0);  // We use: layout(location=0) and vec3 position;
    gl.glEnableVertexAttribArray(1);  // We use: layout(location=1) and vec4 color;
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             reinterpret_cast<void *>(object->vertexBufferOffset + offsetof(Vertex, position)));
    gl.glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             reinterpret_cast<void *>(object->vertexBufferOffset + offsetof(Vertex, color)));

    // Add to object selection dropdown
    objectSelection->addItem(object->name);
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
        gl.glBindVertexArray(object.vertexArrayObject);

        // Model matrix (object movement)
        glm::mat4 M = glm::mat4(1);
        M = glm::translate(M, object.translation);
        M = glm::rotate(M, object.rotation.x, glm::vec3(1, 0, 0));
        M = glm::rotate(M, object.rotation.y, glm::vec3(0, 0, 1));
        M = glm::rotate(M, object.rotation.z, glm::vec3(0, 1, 0));
        M = glm::scale(M, object.scale);

        // Final render matrix
        glm::mat4 PVM = P * V * M;

        // Uniforms
        gl.glUniformMatrix4fv(gl.glGetUniformLocation(programShaderID, "PVM"), 1, GL_FALSE, glm::value_ptr(PVM));

        // Draw
        gl.glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(object.indices.size()), GL_UNSIGNED_INT,
                          reinterpret_cast<void *>(object.indexBufferOffset));
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

void WidgetOpenGLDraw::setObject(int index) {
    selectedObject = &objects.at(static_cast<uint32_t>(index));
}

bool WidgetOpenGLDraw::loadObject(QString fileName) {
    // TODO Load from actual file
    objects.push_back(makePyramid(glm::vec3(-1, 3, -1), 3, fileName));

    // Update Vertex and Index (Element) Buffers and generate Vertex Array Object for new object
    updateBuffers();
    generateObjectVAO(&objects.back());

    // Select new object
    objectSelection->setCurrentIndex(static_cast<int>(objects.size() - 1));

    update(); // Redraw scene

    return true;
}

Object WidgetOpenGLDraw::makeCube(glm::vec3 baseVertex, GLuint baseIndex) {
    std::uniform_real_distribution<float> dist(0, 1);
    glm::vec4 rngColor = glm::vec4(dist(rng), dist(rng), dist(rng), 1);

    Object cube = {
        {
            {baseVertex,                      rngColor},
            {baseVertex + glm::vec3(1, 0, 0), rngColor},
            {baseVertex + glm::vec3(1, 0, 1), rngColor},
            {baseVertex + glm::vec3(0, 0, 1), rngColor},
            {baseVertex + glm::vec3(0, 1, 0), rngColor},
            {baseVertex + glm::vec3(1, 1, 0), rngColor},
            {baseVertex + glm::vec3(1, 1, 1), rngColor},
            {baseVertex + glm::vec3(0, 1, 1), rngColor}
        },
        {
            baseIndex + 0, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex + 0,
            baseIndex + 4, baseIndex + 5, baseIndex + 6, baseIndex + 6, baseIndex + 7, baseIndex + 4,
            baseIndex + 0, baseIndex + 1, baseIndex + 5, baseIndex + 5, baseIndex + 4, baseIndex + 0,
            baseIndex + 1, baseIndex + 2, baseIndex + 6, baseIndex + 6, baseIndex + 5, baseIndex + 1,
            baseIndex + 2, baseIndex + 3, baseIndex + 7, baseIndex + 7, baseIndex + 6, baseIndex + 2,
            baseIndex + 3, baseIndex + 0, baseIndex + 7, baseIndex + 7, baseIndex + 4, baseIndex + 0
        }
    };

    return cube;
}

Object WidgetOpenGLDraw::makePyramid(glm::vec3 baseVertex, uint32_t rows, QString name) {
    Object pyramid(name);

    float offset = 0.0f;
    for (uint32_t row = 0; row < rows; ++row) {
        for (uint32_t i = 0; i < rows - row; ++i) {
            for (uint32_t j = 0; j < rows - row; ++j) {
                // Make cube and merge it into the pyramid
                Object cube = makeCube(baseVertex + glm::vec3(offset + i, row, offset + j), static_cast<GLuint>(pyramid.vertices.size()));
                pyramid.vertices.insert(std::end(pyramid.vertices), std::begin(cube.vertices), std::end(cube.vertices));
                pyramid.indices.insert(std::end(pyramid.indices), std::begin(cube.indices), std::end(cube.indices));
            }
        }

        offset += 0.5f;
    }

    return pyramid;
}
