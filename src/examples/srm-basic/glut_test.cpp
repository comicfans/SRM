#include <iostream>
#include <memory>
#include <vector>

#include <GLES2/gl2.h>
#include <algorithm>
#include <GL/freeglut.h>
#include "qrcodegen.hpp"
#include "qr_paint.hpp"


std::shared_ptr<QrPaint> ptr;

int windowWidth = 512, windowHeight = 512;

// --- Shaders ---






// --- The Reshape Logic ---
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    // Update the viewport to the full new window size
    glViewport(0, 0, w, h);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    ptr->drawQRCode("https://www.google.com", windowWidth, windowHeight,0, 2,2);
    ptr->drawQRCode("https://www.google.com", windowWidth, windowHeight,3, 2,2);

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(512, 512);
    glutCreateWindow("Scalable QR Code GLES2");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Set clear color to white
    ptr.reset(new QrPaint());

    glutDisplayFunc(display);
    glutReshapeFunc(reshape); // Register the resize callback
    
    glutMainLoop();
    return 0;
}
