#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <GL/glut.h>
#include "TP_STEREO.h"

/* Application OpenGL d'images stéréo en utilisant un tampond'accumulation */

/* Variables */
int forme = 0; // Type d'objet/forme à afficher
float rotatespeed = 0.5; // Pour faire tourner les objets
double dtheta = 1.0; // Augmentation de l'angle de rotation
CAMERA camera; // Type CAMERA
XYZ origin = { 0.0,0.0,0.0 }; // Type XYZ
GLfloat pointSize = 1.0f; // Pour augmenter/diminuer la taille des sommets
int eye = 0; // eye = 0 ce sont les deux yeux; eye = 1 c'est l'œil gauche; eye = 2 c'est l'œil droit

/* Structure pour le cube */
typedef struct {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
    //float a;
} vertex;

/* Création des sommets du cube blanc */
vertex cube[8] = {
   {-3.0f,-3.0f, 3.0f,1.0f,1.0f,1.0f},
   {-3.0f, 3.0f, 3.0f,1.0f,1.0f,1.0f},
   { 3.0f, 3.0f, 3.0f,1.0f,1.0f,1.0f},
   { 3.0f,-3.0f, 3.0f,1.0f,1.0f,1.0f},
   {-3.0f,-3.0f,-3.0f,1.0f,1.0f,1.0f},
   {-3.0f, 3.0f,-3.0f,1.0f,1.0f,1.0f},
   { 3.0f, 3.0f,-3.0f,1.0f,1.0f,1.0f},
   { 3.0f,-3.0f,-3.0f,1.0f,1.0f,1.0f}
};

/* Création des faces du  */
int face[6][4] = {
   {0,1,2,3},
   {3,2,6,7},
   {4,5,6,7},
   {0,1,5,4},
   {1,5,6,2},
   {0,4,7,3}
};


/* MAIN */
int main(int argc, char** argv)
{
    // Initialisations du GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_ACCUM | GLUT_RGB | GLUT_DEPTH);

    // Création de la fenêtre et des handlers
    glutCreateWindow("Stereo");
    camera.screenwidth = 400;
    camera.screenheight = 300;
    glutReshapeWindow(camera.screenwidth, camera.screenheight);
    glutDisplayFunc(HandleDisplay);
    glutReshapeFunc(HandleReshape);
    glutVisibilityFunc(HandleVisibility);
    glutKeyboardFunc(HandleKeyboard);
    glutSpecialFunc(HandleSpecialKeyboard);
    CreateEnvironment();
    CameraHome();
    glutMainLoop();
    return(0);
}

/* Configuration constante d'OpenGL et de GLUT */
void CreateEnvironment(void)
{
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearAccum(0.0, 0.0, 0.0, 0.0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

/* Affichages pour chaque œil et leurs projections */
void HandleDisplay(void)
{
    int i, j;
    XYZ r;
    double dist, ratio, radians, scale, wd2, ndfl;
    double left, right, top, bottom, near = 0.1, far = 10000;

    near = camera.focallength / 5; // Pour éviter la stéréo extrême

    // Dériver le vecteur de droite
    CROSSPROD(camera.vd, camera.vu, r);
    Normalise(&r);
    r.x *= camera.eyesep / 2.0;
    r.y *= camera.eyesep / 2.0;
    r.z *= camera.eyesep / 2.0;

    ratio = camera.screenwidth / (double)camera.screenheight;
    radians = DTOR * camera.aperture / 2;
    wd2 = near * tan(radians);
    ndfl = near / camera.focallength;

    // Buffer pour écrire et lecture
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);

    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_ACCUM_BUFFER_BIT); /* Not strictly necessary */

    glViewport(0, 0, camera.screenwidth, camera.screenheight);

    // Filter œil gauche
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);

    // Projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    left = -ratio * wd2 + 0.5 * camera.eyesep * ndfl;
    right = ratio * wd2 + 0.5 * camera.eyesep * ndfl;
    top = wd2;
    bottom = -wd2;
    glFrustum(left, right, bottom, top, near, far);

    // Modèle gauche
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camera.vp.x - r.x, camera.vp.y - r.y, camera.vp.z - r.z,
        camera.vp.x - r.x + camera.vd.x,
        camera.vp.y - r.y + camera.vd.y,
        camera.vp.z - r.z + camera.vd.z,
        camera.vu.x, camera.vu.y, camera.vu.z);
    if (eye == 0 || eye == 1) // Les deux yeux || l'œil gauche !!!
        CreateWorld();
    glFlush();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Écriture sur le tampon d'accumulation
    glAccum(GL_LOAD, 1.0);

    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    left = -ratio * wd2 - 0.5 * camera.eyesep * ndfl;
    right = ratio * wd2 - 0.5 * camera.eyesep * ndfl;
    top = wd2;
    bottom = -wd2;
    glFrustum(left, right, bottom, top, near, far);

    // Filter œil droite
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);

    // Modèle droite
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camera.vp.x + r.x, camera.vp.y + r.y, camera.vp.z + r.z,
        camera.vp.x + r.x + camera.vd.x,
        camera.vp.y + r.y + camera.vd.y,
        camera.vp.z + r.z + camera.vd.z,
        camera.vu.x, camera.vu.y, camera.vu.z);
    if (eye == 0 || eye == 2) // Les deux yeux || l'œil droit !!!
        CreateWorld();
    glFlush();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Copie de l'image et du résultat
    glAccum(GL_ACCUM, 1.0);
    glAccum(GL_RETURN, 1.0);

    glutSwapBuffers();
}

/* Création du modèle qui tourne autour de l'axe y */
void CreateWorld(void)
{
    static double rotateangle = 0.0;

    glPushMatrix();
    glRotatef(rotateangle, 0.0, 1.0, 0.0);
    if (forme == 1)
        MakeDisk();
    else if (forme == 2)
        MakeHLine();
    else if (forme == 3)
        MakeVLine();
    else
        MakeCube();
    glPopMatrix();

    rotateangle += rotatespeed;
}

/* Dessin du cube face par face */
void MakeCube(void)
{
    for (int i = 0; i < 6; i++) {
        glBegin(GL_POLYGON);
        for (int j = 0; j < 4; j++) {
            glColor3f(cube[face[i][j]].r, cube[face[i][j]].g, cube[face[i][j]].b);
            glVertex3f(cube[face[i][j]].x, cube[face[i][j]].y, cube[face[i][j]].z);
        }
        glEnd();
    }
}

/* Dessin du disque */
void MakeDisk(void)
{
    float angle = 0.0f, rayon = 3.0f;
    int nb_faces = 100;
    glBegin(GL_POLYGON);
    for (int i = 0; i < nb_faces; i++)
    {
        angle = 2 * M_PI * i / nb_faces;
        glVertex2f(cos(angle) * rayon, sin(angle) * rayon);
    }
    glEnd();
}

/* Dessin de la ligne horizontale */
void MakeHLine(void)
{
    XYZ pmin = { -3,-3,-3 }, pmax = { 3,3,3 };

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex3f(pmin.x, pmax.y, pmax.z);
    glVertex3f(pmin.x, pmax.y, pmin.z);
    glEnd();
}

/* Dessin de la ligne verticale */
void MakeVLine(void)
{
    XYZ pmin = { -3,-3,-3 }, pmax = { 3,3,3 };
    glBegin(GL_LINES);
    glVertex3f(pmax.x, pmin.y, pmin.z); glVertex3f(pmax.x, pmax.y, pmin.z);
    glEnd();
}

/* Gestion des touches par clavier */
void HandleKeyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'Q':  // Quit
    case 'q':
    case 27:
        exit(0);
        break;
    case 'o': // Home
    case 'O':
        CameraHome();
        break;
    case 'u': // Translater camera UP
    case 'U':
        TranslateCamera(0, 1);
        break;
    case 'd': // Translater camera DOWN
    case 'D':
        TranslateCamera(0, -1);
        break;
    case 'l': // Translater camera LEFT
    case 'L':
        TranslateCamera(-1, 0);
        break;
    case 'r': // Translater camera RIGHT
    case 'R':
        TranslateCamera(1, 0);
        break;
    case 'p': // Plein
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    case 'f': // Fil de fer
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case 's': // Sommets
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case '+': // Augmenter la taille des sommets affiches
        pointSize += 1.0f;
        glPointSize(pointSize);
        break;
    case '-': // Diminuer la taille des sommets affiches
        pointSize -= 1.0f;
        if (pointSize <= 0.0f)
            pointSize = 1.0f;
        glPointSize(pointSize);
        break;
    case 'c': // CUBE
    case 'C':
        forme = 0;
        break;
    case '.': // POINT
        forme = 1;
        break;
    case 'h': // LIGNE HORIZONTALE
    case 'H':
        forme = 2;
        break;
    case 'v': // LIGNE VERTICALE
    case 'V':
        forme = 3;
        break;
    }
}

/* Gestion des touches par clavier pour la rotation */
void HandleSpecialKeyboard(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_LEFT:
        RotateCamera(-1, 0, 0);
        break;
    case GLUT_KEY_RIGHT:
        RotateCamera(1, 0, 0);
        break;
    case GLUT_KEY_UP:
        RotateCamera(0, 1, 0);
        break;
    case GLUT_KEY_DOWN:
        RotateCamera(0, -1, 0);
        break;
    }
}

/* Rotation de la caméra */
void RotateCamera(int ix, int iy, int iz)
{
    XYZ vp, vu, vd;
    XYZ right;
    XYZ newvp, newr;
    double radius, dd, radians;
    double dx, dy, dz;

    vu = camera.vu;
    Normalise(&vu);
    vp = camera.vp;
    vd = camera.vd;
    Normalise(&vd);
    CROSSPROD(vd, vu, right);
    Normalise(&right);
    radians = dtheta * M_PI / 180.0;

    // Gérer la rotation dans Z
    if (iz != 0) {
        camera.vu.x += iz * right.x * radians;
        camera.vu.y += iz * right.y * radians;
        camera.vu.z += iz * right.z * radians;
        Normalise(&camera.vu);
        return;
    }

    // Distance depuis le point de rotation
    dx = camera.vp.x - camera.pr.x;
    dy = camera.vp.y - camera.pr.y;
    dz = camera.vp.z - camera.pr.z;
    radius = sqrt(dx * dx + dy * dy + dz * dz);

    // Nouveau point de vue
    dd = radius * radians;
    newvp.x = vp.x + dd * ix * right.x + dd * iy * vu.x - camera.pr.x;
    newvp.y = vp.y + dd * ix * right.y + dd * iy * vu.y - camera.pr.y;
    newvp.z = vp.z + dd * ix * right.z + dd * iy * vu.z - camera.pr.z;
    Normalise(&newvp);
    camera.vp.x = camera.pr.x + radius * newvp.x;
    camera.vp.y = camera.pr.y + radius * newvp.y;
    camera.vp.z = camera.pr.z + radius * newvp.z;

    // Vecteur de droite
    newr.x = camera.vp.x + right.x - camera.pr.x;
    newr.y = camera.vp.y + right.y - camera.pr.y;
    newr.z = camera.vp.z + right.z - camera.pr.z;
    Normalise(&newr);
    newr.x = camera.pr.x + radius * newr.x - camera.vp.x;
    newr.y = camera.pr.y + radius * newr.y - camera.vp.y;
    newr.z = camera.pr.z + radius * newr.z - camera.vp.z;

    camera.vd.x = camera.pr.x - camera.vp.x;
    camera.vd.y = camera.pr.y - camera.vp.y;
    camera.vd.z = camera.pr.z - camera.vp.z;
    Normalise(&camera.vd);

    // Nouveau vecteur UP
    CROSSPROD(newr, camera.vd, camera.vu);
    Normalise(&camera.vu);
}

/* Translater la caméra */
void TranslateCamera(int ix, int iy)
{
    XYZ vp, vu, vd;
    XYZ right;
    XYZ newvp, newr;
    double radians, delta;

    vu = camera.vu;
    Normalise(&vu);
    vp = camera.vp;
    vd = camera.vd;
    Normalise(&vd);
    CROSSPROD(vd, vu, right);
    Normalise(&right);
    radians = dtheta * M_PI / 180.0;
    delta = dtheta * camera.focallength / 90.0;

    camera.vp.x += iy * vu.x * delta;
    camera.vp.y += iy * vu.y * delta;
    camera.vp.z += iy * vu.z * delta;
    camera.pr.x += iy * vu.x * delta;
    camera.pr.y += iy * vu.y * delta;
    camera.pr.z += iy * vu.z * delta;

    camera.vp.x += ix * right.x * delta;
    camera.vp.y += ix * right.y * delta;
    camera.vp.z += ix * right.z * delta;
    camera.pr.x += ix * right.x * delta;
    camera.pr.y += ix * right.y * delta;
    camera.pr.z += ix * right.z * delta;
}

/* Mise en place de la visibilité */
void HandleVisibility(int visible)
{
    if (visible == GLUT_VISIBLE)
        HandleTimer(0);
    else
        ;
}

/* Événement Timer */
void HandleTimer(int value)
{
    glutPostRedisplay();
    glutTimerFunc(30, HandleTimer, 0);
}

/* Redimensionnement de l'écran */
void HandleReshape(int w, int h)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    camera.screenwidth = w;
    camera.screenheight = h;
}

/* Réinitialisation de la caméra */
void CameraHome(void)
{
    camera.aperture = 60;
    camera.pr = origin;

    camera.vd.x = 1;
    camera.vd.y = 0;
    camera.vd.z = 0;

    camera.vu.x = 0;
    camera.vu.y = 1;
    camera.vu.z = 0;

    camera.vp.x = -10;
    camera.vp.y = 0;
    camera.vp.z = 0;

    camera.focallength = 10;

    camera.eyesep = camera.focallength / 30.0;
}

/* Normalisation d'un vecteur */
void Normalise(XYZ* p)
{
    double length;

    length = sqrt(p->x * p->x + p->y * p->y + p->z * p->z);
    if (length != 0) {
        p->x /= length;
        p->y /= length;
        p->z /= length;
    }
    else {
        p->x = 0;
        p->y = 0;
        p->z = 0;
    }
}
