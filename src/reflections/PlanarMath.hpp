
#pragma once
#include <cmath>

namespace WaterReflections::PlanarMath {
struct Mirror {
    float plane[4];
    float clip[4];
    float camera[16];
};

inline float Dot(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline bool MirrorCamera(const float* camera, float radius, Mirror& out) {
    const float* eye = camera + 12;
    const float* forward = camera + 4;
    float eye2 = Dot(eye, eye);
    if (!std::isfinite(eye2) || !(radius > 1) || eye2 <= (radius + 0.05f) * (radius + 0.05f))
        return false;
    float eyeLength = std::sqrt(eye2);
    float nadir[3] = {eye[0] / eyeLength, eye[1] / eyeLength, eye[2] / eyeLength};
    float horizon = std::acos(radius / eyeLength);
    float height = eyeLength - radius;
    float inside = (2 * radius * height + height * height) / (4 * radius) * 1.1f;
    float limitCosine = (radius + inside) / eyeLength;
    float limit = limitCosine < 1 ? std::acos(limitCosine) : 0;
    float b = Dot(eye, forward);
    float discriminant = b * b - eye2 + radius * radius;
    float distance = discriminant >= 0 ? -b - std::sqrt(discriminant) : -1;
    float normal[3], angle = limit;
    bool anchored = false;
    if (distance > 0) {
        for (unsigned i = 0; i < 3; ++i)
            normal[i] = (eye[i] + distance * forward[i]) / radius;
        float cosine = Dot(normal, nadir);
        angle = std::acos(cosine < 1 ? cosine : 1);
        anchored = angle <= limit;
    }
    if (!anchored) {
        float along = Dot(forward, nadir);
        float tangent[3] = {forward[0] - along * nadir[0], forward[1] - along * nadir[1],
                            forward[2] - along * nadir[2]};
        float length = std::sqrt(Dot(tangent, tangent));
        if (length < 1.0e-4f) {
            for (unsigned i = 0; i < 3; ++i)
                normal[i] = nadir[i];
            angle = 0;
        } else {
            float c = std::cos(limit), s = std::sin(limit) / length;
            for (unsigned i = 0; i < 3; ++i)
                normal[i] = c * nadir[i] + s * tangent[i];
            angle = limit;
        }
    }
    float length = std::sqrt(Dot(normal, normal));
    if (!std::isfinite(length) || length < 0.5f)
        return false;
    for (unsigned i = 0; i < 3; ++i)
        out.plane[i] = out.clip[i] = normal[i] / length;
    out.plane[3] = -radius;
    float reach = angle + horizon;
    out.clip[3] = -radius * (reach < 1.4f ? std::cos(reach) : std::cos(1.4f));
    for (unsigned row = 0; row < 4; ++row) {
        float d = Dot(camera + row * 4, out.plane) + (row == 3 ? out.plane[3] : 0);
        float sign = row == 0 ? -1.0f : 1.0f;
        for (unsigned col = 0; col < 3; ++col)
            out.camera[row * 4 + col] = sign * (camera[row * 4 + col] - 2 * d * out.plane[col]);
        out.camera[row * 4 + 3] = camera[row * 4 + 3];
    }
    return true;
}

inline void Multiply(const float* a, const float* b, float* result) {
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col) {
            result[row * 4 + col] = 0;
            for (unsigned k = 0; k < 4; ++k)
                result[row * 4 + col] += a[row * 4 + k] * b[k * 4 + col];
        }
}

inline bool Inverse(const float* matrix, float* inverse) {
    double a[4][8]{};
    for (unsigned row = 0; row < 4; ++row) {
        for (unsigned col = 0; col < 4; ++col)
            a[row][col] = matrix[row * 4 + col];
        a[row][row + 4] = 1;
    }
    for (unsigned col = 0; col < 4; ++col) {
        unsigned pivot = col;
        for (unsigned row = col + 1; row < 4; ++row)
            if (std::fabs(a[row][col]) > std::fabs(a[pivot][col]))
                pivot = row;
        if (std::fabs(a[pivot][col]) < 1.0e-12)
            return false;
        for (unsigned k = 0; k < 8; ++k) {
            double swap = a[col][k];
            a[col][k] = a[pivot][k];
            a[pivot][k] = swap;
        }
        double divisor = a[col][col];
        for (unsigned k = 0; k < 8; ++k)
            a[col][k] /= divisor;
        for (unsigned row = 0; row < 4; ++row) {
            if (row == col)
                continue;
            double scale = a[row][col];
            for (unsigned k = 0; k < 8; ++k)
                a[row][k] -= scale * a[col][k];
        }
    }
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col)
            inverse[row * 4 + col] = float(a[row][col + 4]);
    return true;
}

inline bool ClipPlane(const float* viewProjection, const float* plane, float* clip) {
    float inverse[16];
    if (!Inverse(viewProjection, inverse))
        return false;
    float largest = 0;
    for (unsigned row = 0; row < 4; ++row) {
        clip[row] = 0;
        for (unsigned col = 0; col < 4; ++col)
            clip[row] += inverse[row * 4 + col] * plane[col];
        if (std::fabs(clip[row]) > largest)
            largest = std::fabs(clip[row]);
    }
    if (!std::isfinite(largest) || largest < 1.0e-12f)
        return false;
    for (unsigned i = 0; i < 4; ++i)
        clip[i] /= largest;
    return true;
}
}

