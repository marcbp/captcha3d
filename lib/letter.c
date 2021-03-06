#include "letter.h"
#include "data.h"

#include <math.h>
#include <stdio.h>

static void initialize_normal_vector(Vector3d array[], int size)
{
    Vector3d nul;
    int i;

    nul.x = 0;
    nul.y = 0;
    nul.z = 0;

    for (i = 0; i < size; i++) {
        array[i] = nul;
    }
}

static Vector3d letter_apply_matrix(Vector3d m[3], Vector3d q)
{
    Vector3d p = {
        p.x = m[0].x * q.x + m[1].x * q.y + m[2].x * q.z ,
        p.y = m[0].y * q.x + m[1].y * q.y + m[2].y * q.z ,
        p.z = m[0].z * q.x + m[1].z * q.y + m[2].z * q.z
    };

    return p;
}

static float distance_letter(float z, int width, int marge)
{
    return (float) width / 2 - (fabs(z) + Z_PROJECTION_CENTER) / Z_PROJECTION_CENTER * (width / 2 - marge);
}

static Vector3d compute_translation(int width, int height, int marge, float z)
{
    Vector3d translate;

    translate.x = distance_letter(z, width, marge);
    translate.y = height / 2;
    translate.z = z;

    return translate;
}

void letter_translate(Letter *letter, Vector3d vector)
{
    for (size_t i = 0; i < letter->pointsNumber; i++) {
        letter->points[i].x += vector.x;
        letter->points[i].y += vector.y;
        letter->points[i].z += vector.z;

        if (letter->points[i].z < 0) {
            fprintf(stderr, "Warning: a letter is crossing the screen !\n");
        }
    }
}

void letter_rotate(Letter *letter, Vector3d angles)
{
    Vector3d rotx[3] = {{1, 0, 0}, {0, cos(angles.x), -sin(angles.x)}, {0, sin(angles.x), cos(angles.x)}} ;
    Vector3d roty[3] = {{cos(angles.y), 0, sin(angles.y)}, {0, 1, 0}, { -sin(angles.y), 0, cos(angles.y)}} ;
    Vector3d rotz[3] = {{cos(angles.z), -sin(angles.z), 0}, {sin(angles.z), cos(angles.z), 0}, {0, 0, 1}} ;

    for (int i = 0; i < letter->pointsNumber; i++) {
        letter->points[i] = letter_apply_matrix(rotx, letter->points[i]) ;
        letter->points[i] = letter_apply_matrix(roty, letter->points[i]) ;
        letter->points[i] = letter_apply_matrix(rotz, letter->points[i]) ;
    }
}

void letter_scale(Letter *letter, float coeff)
{
    for (int i = 0; i < letter->pointsNumber; i++) {
        letter->points[i].x *= coeff;
        letter->points[i].y *= coeff;
        letter->points[i].z *= coeff;
    }
}

void letter_transform(Letter *letter, int i, int width, int height, int z, int marge, float e, float offset)
{
    Vector3d translate;
    float angle1, angle2, angle3;

    letter_scale(letter, e) ;

    angle1 = (float) 1.5 * rand() / RAND_MAX - 0.75;
    angle2 = (float) 1.5 * rand() / RAND_MAX - 0.75;
    angle3 = (float) 1.5 * rand() / RAND_MAX - 0.75;

    Vector3d angles = {angle1, angle2, angle3} ;
    letter_rotate(letter, angles);

    translate = compute_translation(width, height, marge, z);

    translate.x += offset * i;
    letter_translate(letter, translate);
}

void compute_normal_vectors(Vector3d tab[], const Letter *letter)
{
    Triangle face;
    initialize_normal_vector(tab, letter->pointsNumber);

    for (size_t i = 0; i < letter->facesNumber; i++) {
        face = letter->faces[i];

        Vector3d normal = face_normal_vector(&letter->faces[i], letter->points);

        tab[face.a] = vector_add(&tab[face.a], &normal);
        tab[face.b] = vector_add(&tab[face.b], &normal);
        tab[face.c] = vector_add(&tab[face.c], &normal);
    }

    for (size_t i = 0; i < letter->pointsNumber; i++) {
        tab[i] = vector_normalize(&tab[i]);
    }
}

Vector3d face_normal_vector(const Triangle *face, const Vector3d points[])
{
    Vector3d n;

    const Vector3d *p1 = &points[face->a];
    const Vector3d *p2 = &points[face->b];
    const Vector3d *p3 = &points[face->c];

    n.x = -((p2->y - p1->y) * (p3->z - p1->z) - (p2->z - p1->z) * (p3->y - p1->y));
    n.y = -((p2->z - p1->z) * (p3->x - p1->x) - (p2->x - p1->x) * (p3->z - p1->z));
    n.z = -((p2->x - p1->x) * (p3->y - p1->y) - (p2->y - p1->y) * (p3->x - p1->x));

    return vector_normalize(&n);
}
