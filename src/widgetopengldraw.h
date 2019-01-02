#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <fstream>

#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QTime>
#include <QMouseEvent>
#include <QComboBox>
#include <QFileInfo>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
};

struct Material {
    // TODO Below property modifications in UI
    glm::vec3 ambientColor = glm::vec3(0.1f);
    glm::vec3 diffuseColor = glm::vec3(0.5f);
    glm::vec3 specularColor = glm::vec3(1.0f);
    float specularPower = 10.0f; // Shininess factor
};

struct Object {
    QString name;

    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    Object(QString name_)
        : name(name_) {}
};

struct MeshObject : Object {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    Material material;

    QImage textureImage;
    QImage bumpMapImage;

    GLuint VAO; // Vertex Array Object
    GLuint VBO; // Vertex Buffer Object
    GLuint IBO; // Index Buffer Object
    GLuint TBO[2]; // Texture Buffer Object (Texture, Bump Map)

    MeshObject(QString name_)
        : Object(name_), vertices({}), indices({}) {}
    MeshObject(QString name_, std::vector<Vertex> vertices_, std::vector<GLuint> indices_)
        : Object(name_), vertices(vertices_), indices(indices_) {}
};

struct LightObject : Object {
    // position = MeshObject.translation
    // power = MeshObject.scale
    glm::vec3 color = glm::vec3(1.0f);

    LightObject()
        : Object("") {}

    LightObject(QString name_)
        : Object(name_) {}

    LightObject(QString name_, glm::vec3 position_ = {0.0f, 0.0f, 0.0f}, float power_ = 40.0f)
        : Object(name_) {
        translation = position_;
        scale = glm::vec3(power_);
    }
};

class QOpenGLFunctions_3_3_Core;

class WidgetOpenGLDraw : public QOpenGLWidget {
    Q_OBJECT
public:
    QComboBox *objectSelection;

    Object *selectedObject;
    LightObject light; // Single light support only

    WidgetOpenGLDraw(QWidget* parent);
    ~WidgetOpenGLDraw() override;

    bool isMeshObjectSelected();

    // Input
    void handleKeys(QSet<int> keys, Qt::KeyboardModifiers modifiers);

    // Loaders
    void loadModelsFromFile(QStringList &paths, bool preload = false);
    void applyTextureFromFile(QString path, MeshObject *object = nullptr, bool preload = false);
    void applyBumpMapFromFile(QString path, MeshObject *object = nullptr, bool preload = false);

    // Generators
    MeshObject makeCube(QString name = "");
    MeshObject makePyramid(uint32_t rows, QString name = "");

public slots:
    void selectObject(int index);

protected:
    // OpenGL overrides
    void paintGL() override;
    void initializeGL() override;
    void resizeGL(int w, int h) override;

    // Buffers
    void generateObjectBuffers(MeshObject &object);
    void loadObjectTexture(MeshObject &object);
    void loadObjectBumpMap(MeshObject &object);

private:
    QOpenGLFunctions_3_3_Core gl;
    std::mt19937 rng;

    // Shaders
    static const GLchar* vertexShaderSource;
    static const GLchar* fragmentShaderSource;
    GLuint programShaderID;
    GLuint vertexShaderID;
    GLuint fragmentShaderID;

    std::vector<MeshObject> objects;

    // Initial camera position
    glm::vec3 cameraPos = glm::vec3(6.5f, 5.5f, -10.0f);
    float cameraPitch = -15.0f;
    float cameraYaw = -32.0f;
    float cameraSpeed = 0.1f;
    float cameraSensitivity = 0.1f;
    // Other camera variables
    QPoint mousePos;
    glm::vec3 cameraFront = glm::vec3(0);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    bool projectionOrtho = false;

    // Shaders
    void compileShaders();
    void printProgramInfoLog(GLuint obj);
    void printShaderInfoLog(GLuint obj);

    // Input
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void updateCameraFront();

    // Format loaders
    bool loadModelOBJ(const char *path, MeshObject &object /* out */);

    // Generators
    MeshObject makeCubeOffset(glm::vec3 baseVertex, GLuint baseIndex = 0, QString name = "");
};
