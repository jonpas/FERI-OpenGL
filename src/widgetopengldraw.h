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
    glm::vec4 color;
    glm::vec2 uv;
    glm::vec3 normal;
};

struct Object {
    QString name;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<GLuint> uvIndices;
    std::vector<GLuint> normalIndices;

    GLuint VAO; // Vertex Array Object
    GLuint VBO; // Vertex Buffer Object
    GLuint IBO; // Index Buffer Object

    glm::vec3 translation = glm::vec3(0);
    glm::vec3 scale = glm::vec3(1);
    glm::vec3 rotation = glm::vec3(0);

    Object()
        : name(""), vertices({}), indices({}), uvIndices({}), normalIndices({}) {}
    Object(QString name_)
        : name(name_), vertices({}), indices({}), uvIndices({}), normalIndices({}) {}
    Object(std::vector<Vertex> vertices_, std::vector<GLuint> indices_, std::vector<GLuint> uvIndices_, std::vector<GLuint> normalIndices_)
        : name(""), vertices(vertices_), indices(indices_), uvIndices(uvIndices_), normalIndices(normalIndices_) {}
    Object(QString name_, std::vector<Vertex> vertices_, std::vector<GLuint> indices_, std::vector<GLuint> uvIndices_, std::vector<GLuint> normalIndices_)
        : name(name_), vertices(vertices_), indices(indices_), uvIndices(uvIndices_), normalIndices(normalIndices_) {}
};

class QOpenGLFunctions_3_3_Core;

class WidgetOpenGLDraw : public QOpenGLWidget {
    Q_OBJECT
public:
    QComboBox *objectSelection;

    WidgetOpenGLDraw(QWidget* parent);
    ~WidgetOpenGLDraw() override;

    void handleKeys(QSet<int> keys, Qt::KeyboardModifiers modifiers);
    void loadModelsFromFile(QStringList &paths);

    Object makeCube(glm::vec3 baseVertex, GLuint baseIndex = 0);
    Object makePyramid(glm::vec3 baseVertex, uint32_t rows, QString name = "");

public slots:
    void selectObject(int index);

protected:
    // OpenGL overrides
    void paintGL() override;
    void initializeGL() override;
    void resizeGL(int w, int h) override;

    void generateObjectBuffers(Object &object);

private:
    QOpenGLFunctions_3_3_Core gl;

    // Shaders
    static const GLchar* vertexShaderSource;
    static const GLchar* fragmentShaderSource;
    GLuint programShaderID;
    GLuint vertexShaderID;
    GLuint fragmentShaderID;

    std::vector<Object> objects;
    Object *selectedObject;

    std::mt19937 rng;

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

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void updateCameraFront();

    // Format loaders
    bool loadModelOBJ(const char *path, Object &object /* out */);
};
