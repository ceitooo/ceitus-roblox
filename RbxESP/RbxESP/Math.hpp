#pragma once
#include "Types.hpp"
#include <cmath>

// Retorna true si el punto está FRENTE a la cámara (pw > 0).
// Siempre escribe las coordenadas de pantalla, aunque estén fuera del área visible.
// El caller debe chequear si está dentro de [0,sw]x[0,sh] para dibujar texto/box.
inline bool WorldToScreen(const ViewMatrix_t& vp, Vector3 w, Vector2& out, float sw, float sh) {
    float px = vp.m[0][0] * w.x + vp.m[0][1] * w.y + vp.m[0][2] * w.z + vp.m[0][3];
    float py = vp.m[1][0] * w.x + vp.m[1][1] * w.y + vp.m[1][2] * w.z + vp.m[1][3];
    float pw = vp.m[3][0] * w.x + vp.m[3][1] * w.y + vp.m[3][2] * w.z + vp.m[3][3];

    if (pw < 0.1f) return false;

    out.x = (px / pw) * (sw * 0.5f) + (sw * 0.5f);
    out.y = -(py / pw) * (sh * 0.5f) + (sh * 0.5f);
    return true; // frente a la cámara, coords pueden estar fuera de pantalla
}

// Conveniencia: true solo si está en pantalla
inline bool OnScreen(Vector2 s, float sw, float sh) {
    return s.x > 0.f && s.x < sw && s.y > 0.f && s.y < sh;
}

inline float Dist3D(Vector3 a, Vector3 b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}
