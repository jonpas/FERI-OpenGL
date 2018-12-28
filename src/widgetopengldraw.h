#pragma once

#include <iostream>
#include <vector>
#include <random>

#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QTime>
#include <QMouseEvent>
#include <QComboBox>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <memory>

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
};

struct Object {
    QString name;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    GLuint vertexBufferOffset;
    GLuint indexBufferOffset;
    GLuint vertexArrayObject;

    glm::vec3 translation = glm::vec3(0);
    glm::vec3 scale = glm::vec3(1);
    glm::vec3 rotation = glm::vec3(0);

    Object() : name(""), vertices({}), indices({}) {}
    Object(QString name_) : name(name_), vertices({}), indices({}) {}
    Object(std::vector<Vertex> vertices_, std::vector<GLuint> indices_) : name(""), vertices(vertices_), indices(indices_) {}
    Object(QString name_, std::vector<Vertex> vertices_, std::vector<GLuint> indices_) : name(name_), vertices(vertices_), indices(indices_) {}

    ssize_t vertexBufferSize() const {
        return static_cast<ssize_t>(vertices.size() * sizeof(Vertex));
    }

    ssize_t indexBufferSize() const {
        return static_cast<ssize_t>(indices.size() * sizeof(GLuint));
    }
};

class QOpenGLFunctions_3_3_Core;

class WidgetOpenGLDraw : public QOpenGLWidget {
    Q_OBJECT
public:
    QComboBox *objectSelection;

    WidgetOpenGLDraw(QWidget* parent);
    ~WidgetOpenGLDraw() override;

    void handleKeys(QSet<int> keys, Qt::KeyboardModifiers modifiers);

    Object makeCube(glm::vec3 baseVertex, GLuint baseIndex = 0);
    Object makePyramid(glm::vec3 baseVertex, uint32_t rows, QString name = "");

public slots:
    void setObject(int index);

protected:
    // OpenGL overrides
    void paintGL() override;
    void initializeGL() override;
    void resizeGL(int w, int h) override;

private:
    QOpenGLFunctions_3_3_Core gl;

    // Shaders
    static const GLchar* vertexShaderSource;
    static const GLchar* fragmentShaderSource;
    GLuint programShaderID;
    GLuint vertexShaderID;
    GLuint fragmentShaderID;

    // Buffers
    GLuint vertexBufferID;
    GLuint indexBufferID;

    std::vector<Object> objects;
    // Object pointers (no other accessors available at this time)
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
};
