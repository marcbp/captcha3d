#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "zbuffer.h"
#include "data.h"
#include "illumination.h"

#define max(a,b) (a>=b?a:b)
#define min(a,b) (a<=b?a:b)

typedef struct {
    Vector2d p;
    float i;
} Projection2d;


static float depth(int x, int y, Vector3d a, Vector3d b, Vector3d c, Vector3d cp)
{
    float t;

    t = (cp.x * a.y * c.z - cp.x * a.y * b.z - cp.x * b.y * c.z + cp.x * b.z * c.y - cp.x * a.z * c.y + cp.x * a.z * b.y - cp.y * c.x * b.z - cp.y * a.x * c.z + cp.y * b.x * c.z - cp.y * b.x * a.z + cp.y * a.x * b.z + cp.y * c.x * a.z + cp.z * a.x * c.y - cp.z * b.x * c.y + cp.z * b.x * a.y - cp.z * c.x * a.y + cp.z * c.x * b.y - cp.z * a.x * b.y + b.y * a.x * c.z - b.z * a.x * c.y + a.y * c.x * b.z - a.y * b.x * c.z + a.z * b.x * c.y - b.y * c.x * a.z)
        /
        (-cp.x * a.y * b.z + cp.x * a.y * c.z - cp.x * b.y * c.z + cp.x * b.z * c.y - cp.x * a.z * c.y + cp.x * a.z * b.y - cp.y * a.x * c.z + cp.y * b.x * c.z - cp.y * c.x * b.z - cp.y * b.x * a.z + cp.y * a.x * b.z + cp.y * c.x * a.z + cp.z * a.x * c.y + cp.z * c.x * b.y - cp.z * c.x * a.y - cp.z * b.x * c.y + cp.z * b.x * a.y - cp.z * a.x * b.y + x * a.y * b.z - x * a.y * c.z + x * b.y * c.z - x * b.z * c.y - x * a.z * b.y + x * a.z * c.y + c.x * b.z * y + a.x * c.z * y - b.x * c.z * y - a.x * b.z * y + b.x * a.z * y - c.x * a.z * y);

    return -cp.z * t + cp.z;
}

static void projection_swap(Projection2d *p1, Projection2d *p2)
{
    Projection2d temp;
    temp.p = (*p1).p;
    temp.i = (*p1).i;
    (*p1).p = (*p2).p;
    (*p1).i = (*p2).i;
    (*p2).p = temp.p;
    (*p2).i = temp.i;
}

static void sort_projection(Projection2d *p1, Projection2d *p2, Projection2d *p3)
{
    if ((*p1).p.y > (*p2).p.y) {
        projection_swap(p1, p2);
    }

    if ((*p2).p.y > (*p3).p.y) {
        projection_swap(p2, p3);
    }

    if ((*p1).p.y > (*p2).p.y) {
        projection_swap(p1, p2);
    }
}

static void line(int *xDebut, int *xFin, struct Image *temp, int y, int limite1, int limite2)
{
    for (int x = limite1; x <= limite2; x++) {
        const struct Color *color= captcha3d_image_get(temp, x, y);

        if (color->red == 0 && color->green == 0 && color->blue == 0) {
            if (*xDebut == -1) {
                *xDebut = x;
            } else {
                *xFin = x;
            }
        }
    }
    if (*xFin == -1) {
        *xFin = *xDebut;
    }
}

static void scan_line(Vector2d contour[], int x1, int y1, int x2, int y2)
{
    int sx, sy, dx1, dy1, dx2, dy2, x, y, m, n, k, cnt;

    sx = x2 - x1;
    sy = y2 - y1;

    if (sx > 0) {
        dx1 = 1;
    } else if (sx < 0) {
        dx1 = -1;
    } else {
        dx1 = 0;
    }

    if (sy > 0) {
        dy1 = 1;
    } else if (sy < 0) {
        dy1 = -1;
    } else {
        dy1 = 0;
    }

    m = abs(sx);
    n = abs(sy);
    dx2 = dx1;
    dy2 = 0;

    if (m < n) {
        m = abs(sy);
        n = abs(sx);
        dx2 = 0;
        dy2 = dy1;
    }

    x = x1;
    y = y1;
    cnt = m + 1;
    k = n / 2;

    while (cnt--) {
        if (y >= 0) {
            if (x < contour[y].x) {
                contour[y].x = x;
            }
            if (x > contour[y].y) {
                contour[y].y = x;
            }
        }

        k += n;
        if (k < m) {
            x += dx2;
            y += dy2;
        } else {
            k -= m;
            x += dx1;
            y += dy1;
        }
    }
}

static void fill_triangle(struct Image *image, struct Color color, Vector2d p1, Vector2d p2, Vector2d p3)
{
    Vector2d contour[image->height];

    for (size_t y = 0; y < image->height; y++) {
        contour[y].x = FLT_MAX;
        contour[y].y = FLT_MIN;
    }

    scan_line(contour, p1.x, p1.y, p2.x, p2.y);
    scan_line(contour, p2.x, p2.y, p3.x, p3.y);
    scan_line(contour, p3.x, p3.y, p1.x, p1.y);

    for (size_t y = 0; y < image->height; y++) {
        if (contour[y].y >= contour[y].x) {
            int x = contour[y].x;
            int len = 1 + contour[y].y - contour[y].x;

            while (len--) {
                struct Color *point = captcha3d_image_get(image, x++, y);
                *point = color;
            }
        }
    }
}

void z_buffer_run(struct zBufferData *buffer, const Letter *letter, Material materiau)
{
    struct Image *temp = buffer->temporary;

    struct Color black = {0, 0, 0, 255};
    int xDebut, xFin, xLimiteG, xLimiteD, xMilieu;
    float ia, ib, ip, alpha, beta, gamma;
    float d1, d2, d3;

    Vector3d normales[500];
    float intensites[500];

    Light lumiere = {0.2, 0.9, {0, 0, 1}};

    Vector3d cp;
    cp.x = buffer->image->width / 2;
    cp.y = buffer->image->height / 2;
    cp.z = -Z_PROJECTION_CENTER;

    compute_normal_vectors(normales, letter);
    color_light_intensity(intensites, normales, letter, lumiere, materiau);

    for (size_t i = 0; i < letter->facesNumber; i++) {
        captcha3d_image_reset(temp);
        const Triangle *face = &letter->faces[i];

        const Vector3d *p1 = &letter->points[face->a];
        const Vector3d *p2 = &letter->points[face->b];
        const Vector3d *p3 = &letter->points[face->c];

        float i1 = intensites[face->a];
        float i2 = intensites[face->b];
        float i3 = intensites[face->c];

        Projection2d pp1 = {vector_project(p1, &cp), i1};
        Projection2d pp2 = {vector_project(p2, &cp), i2};
        Projection2d pp3 = {vector_project(p3, &cp), i3};

        //Ordonnancement des projetés
        sort_projection(&pp1, &pp2, &pp3);

        //Remplissage du triangle
        fill_triangle(temp, black, pp1.p, pp2.p, pp3.p);

        //Détermination de la boite englobant le triangle
        xLimiteG = min(min(pp1.p.x, pp2.p.x), pp3.p.x);
        xLimiteD = max(max(pp1.p.x, pp2.p.x), pp3.p.x);

        // Calcul de xMilieu pour connaitre la configuration du triangle (brisure à gauche ou à droite)
        if (pp3.p.y != pp1.p.y) {
            xMilieu = (pp3.p.x - pp1.p.x) * (pp2.p.y - pp1.p.y) / (pp3.p.y - pp1.p.y) + pp1.p.x;
        } else {
            xMilieu = pp2.p.x;
        }

        for (int k = pp1.p.y; k <= pp3.p.y; k++) {
            xDebut = -1;
            xFin = -1;
            //Calcul des bornes de la ligne
            line(&xDebut, &xFin, temp, k, xLimiteG, xLimiteD);

            int xLimBeta = (pp2.p.x > xMilieu) ? xFin : xDebut;
            int xLimAlpha = (pp2.p.x > xMilieu) ? xDebut : xFin;

            for (int j = xDebut; j <= xFin; j++) {
                //Calcul de la profondeur du point dans l'espace correspondant
                float z = depth(j, k, *p1, *p2, *p3, cp);
                //Récupération de la valeur du zbuffer
                float zbuff = buffer->data[buffer->image->height * j + k];

                //Si le z est plus petit que celui du buffer, ou que le buffer est encore à 0
                if (z <= zbuff || zbuff == 0) {
                    //On calcule l'intensité du point considéré
                    d1 = sqrt((pp1.p.x - pp3.p.x) * (pp1.p.x - pp3.p.x) + (pp1.p.y - pp3.p.y) * (pp1.p.y - pp3.p.y));
                    d2 = sqrt((pp1.p.x - pp2.p.x) * (pp1.p.x - pp2.p.x) + (pp1.p.y - pp2.p.y) * (pp1.p.y - pp2.p.y));
                    d3 = sqrt((pp2.p.x - pp3.p.x) * (pp2.p.x - pp3.p.x) + (pp2.p.y - pp3.p.y) * (pp2.p.y - pp3.p.y));

                    if (d1 != 0) {
                        alpha = sqrt((pp1.p.x - xLimAlpha) * (pp1.p.x - xLimAlpha) + (pp1.p.y - k) * (pp1.p.y - k)) / d1;
                    } else {
                        alpha = 0;
                    }

                    ia = alpha * pp3.i + (1 - alpha) * pp1.i;

                    if (k <= pp2.p.y) {
                        //Première partie du triangle
                        if (d2 != 0) {
                            beta = sqrt((pp1.p.x - xLimBeta) * (pp1.p.x - xLimBeta) + (pp1.p.y - k) * (pp1.p.y - k)) / d2;
                        } else {
                            beta = 0;
                        }
                        ib = beta * pp2.i + (1 - beta) * pp1.i;
                    } else {
                        //Deuxième partie du triangle
                        if (d3 != 0) {
                            beta = sqrt((pp2.p.x - xLimBeta) * (pp2.p.x - xLimBeta) + (pp2.p.y - k) * (pp2.p.y - k)) / d3;
                        } else {
                            beta = 0;
                        }
                        ib = beta * pp3.i + (1 - beta) * pp2.i;
                    }

                    //Calcul final de l'intensité
                    if (xFin != xDebut) {
                        gamma = (float) (xFin - j) / (xFin - xDebut);
                        ip = (pp2.p.x > xMilieu) ? gamma * ia + (1 - gamma) * ib : gamma * ib + (1 - gamma) * ia;
                    } else {
                        ip = ia;
                    }

                    // Set the color for this position
                    struct Color *pixel = captcha3d_image_get(buffer->image, j, buffer->image->height - k);
                    *pixel = color_gouraud(ip, materiau.couleur);

                    // Update z-buffer
                    buffer->data[buffer->image->height * j + k] = z;
                }
            }
        }
    }
}

struct zBufferData* z_buffer_data_allocate(struct Image *image)
{
    struct zBufferData *buffer = calloc(sizeof(struct zBufferData) + image->width * image->height * sizeof(float), 1);
    buffer->image = image;
    buffer->temporary = captcha3d_image_allocate(image->width, image->height);

    return buffer;
}

void z_buffer_data_release(struct zBufferData *buffer)
{
    captcha3d_image_release(buffer->temporary);
    free(buffer);
}
