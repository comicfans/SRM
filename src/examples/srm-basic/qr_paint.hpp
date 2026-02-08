#pragma once

#include <GLES2/gl2.h>
#include "qrcodegen.hpp"
#include <iostream>
#include <vector>

using qrcodegen::QrCode;

struct QrPaint{


  std::pair<std::vector<unsigned char>,int> qrFor(const std::string& message){

    QrCode qr = QrCode::encodeText(message.c_str(), QrCode::Ecc::MEDIUM);
    int size = qr.getSize();


    std::vector<unsigned char> pixels;
    const auto white = "\033[47m  \033[0m";
    const auto black = "\033[40m  \033[0m";
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            pixels.push_back(qr.getModule(x, y) ? 0 : 255);
            //std::cout<<(qr.getModule(x, y) ? black:white );
        }
        //std::cout<<std::endl;
    }

    return {pixels, size};
  }

  void drawQRCode(const std::string& message,float windowWidth, float windowHeight,
      const int index, const int horiCount, const int vertCount) {

    const auto [pixels,size] = qrFor(message);

    // Calculate aspect ratio to keep the QR code square
    float aspect = (float)windowWidth / (float)windowHeight;

    float unitSizeX = 1.0f / std::max(horiCount, vertCount);
    float unitSizeY = 1.0f / std::max(horiCount, vertCount);

    if (aspect > 1.0f) {
        unitSizeX /= aspect; // Window is wider than tall
    } else {
        unitSizeY *= aspect; // Window is taller than wide
    }

    const int xIdx = index % vertCount;
    const int yIdx = index / vertCount;

    float xcenter = unitSizeX * 2 *(xIdx + 0.5 - horiCount/2.0);
    float ycenter = unitSizeY * 2*(-yIdx - 0.5 + vertCount/2.0);

    const float scale = 0.8;

    // Dynamic vertex data based on current aspect ratio
    float vertices[] = {
        xcenter-unitSizeX * scale,  ycenter + unitSizeY * scale,  0.0f, 0.0f,
        xcenter-unitSizeX * scale, ycenter-unitSizeY * scale,  0.0f, 1.0f,
        xcenter+ unitSizeX * scale, ycenter + unitSizeY * scale,  1.0f, 0.0f,
        xcenter+ unitSizeX * scale, ycenter - unitSizeY * scale,  1.0f, 1.0f
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glEnableVertexAttribArray(texLoc);
    glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));


    glBindTexture(GL_TEXTURE_2D, qrTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, size, size, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels.data());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }



  GLuint vbo;
  GLuint qrTexture;
  GLuint shaderProgram;

  const char* vShaderSrc = R"(
    attribute vec2 position;
    attribute vec2 texCoord;
    varying vec2 vTexCoord;
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        vTexCoord = texCoord;
    }
)";

const char* fShaderSrc = R"(
    precision mediump float;
    varying vec2 vTexCoord;
    uniform sampler2D uTexture;
    void main() {
        float val = texture2D(uTexture, vTexCoord).r;
        gl_FragColor = vec4(vec3(val), 1.0);
    }
)";




  GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
      GLint logLength;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
      std::vector<char> log(logLength);
      glGetShaderInfoLog(shader, logLength, nullptr, log.data());
      std::cerr << "QrPaint Error: " << log.data() << std::endl;
      glDeleteShader(shader);
      return 0;
    }
    return shader;
  }

  GLuint createProgram(const char* vSource, const char* fSource) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fShaderSrc);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Check for linking errors
    GLint status;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
      GLint logLength;
      glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
      std::vector<char> log(logLength);
      glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
      std::cerr << "Linker Error: " << log.data() << std::endl;
    }

    // Shader are linked into the program, so we can clean up the individual objects
    glDeleteShader(vs);
    glDeleteShader(fs);

    return shaderProgram;
  }

  QrPaint(){
    createProgram(vShaderSrc, fShaderSrc);

    posLoc = glGetAttribLocation(shaderProgram, "position");
    texLoc = glGetAttribLocation(shaderProgram, "texCoord");

    glGenBuffers(1, &vbo);
    glGenTextures(1, &qrTexture);
    glBindTexture(GL_TEXTURE_2D, qrTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  }

  ~QrPaint(){
    glDeleteProgram(qrProgram);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &qrTexture);
  }

  GLuint qrProgram;
  GLint posLoc;
  GLint  texLoc;
  GLint samplerLoc;
};
