#include "Perimeter.h"
#include <cmath>

static const float PI = 3.14159265f;

Perimeter::Perimeter(float width, float height, float cornerRadius)
    : m_width(width), m_height(height) {
    // Clamp corner radius so straight segments don't become negative
    float maxR = 0.49f * (width < height ? width : height);
    m_cornerRadius = cornerRadius < maxR ? cornerRadius : maxR;
    float R = m_cornerRadius;
    float W = width;
    float H = height;

    // 8 segments: 4 straights (even indices) + 4 arcs (odd indices)
    m_segLen[0] = W - 2.0f * R;      // top straight
    m_segLen[1] = 0.5f * PI * R;     // top-right arc
    m_segLen[2] = H - 2.0f * R;      // right straight
    m_segLen[3] = 0.5f * PI * R;     // bottom-right arc
    m_segLen[4] = W - 2.0f * R;      // bottom straight
    m_segLen[5] = 0.5f * PI * R;     // bottom-left arc
    m_segLen[6] = H - 2.0f * R;      // left straight
    m_segLen[7] = 0.5f * PI * R;     // top-left arc

    m_totalLength = 2.0f * (W + H) - 8.0f * R + 2.0f * PI * R;
}

PerimeterPoint Perimeter::getPointAtProgress(float t) const {
    t = fmod(t, 1.0f);
    if (t < 0.0f) t += 1.0f;
    float dist = t * m_totalLength;
    return getPointAtDistance(dist);
}

PerimeterPoint Perimeter::getPointAtDistance(float dist) const {
    dist = fmod(dist, m_totalLength);
    if (dist < 0.0f) dist += m_totalLength;

    float hw = m_width * 0.5f;
    float hh = m_height * 0.5f;
    float R = m_cornerRadius;

    float cumulative = 0.0f;
    for (int i = 0; i < 8; i++) {
        if (dist <= cumulative + m_segLen[i] + 1e-6f) {
            float local = dist - cumulative;
            float t = (m_segLen[i] > 0.0f) ? local / m_segLen[i] : 0.0f;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            if (i % 2 == 0) {
                // Straight segment
                int edge = i / 2;
                switch (edge) {
                    case 0: // top: left -> right
                        return { glm::vec2(-hw + R + t * (m_width - 2.0f * R), hh),
                                 glm::vec2(1.0f, 0.0f),
                                 glm::vec2(0.0f, 1.0f) };
                    case 1: // right: top -> bottom
                        return { glm::vec2(hw, hh - R - t * (m_height - 2.0f * R)),
                                 glm::vec2(0.0f, -1.0f),
                                 glm::vec2(1.0f, 0.0f) };
                    case 2: // bottom: right -> left
                        return { glm::vec2(hw - R - t * (m_width - 2.0f * R), -hh),
                                 glm::vec2(-1.0f, 0.0f),
                                 glm::vec2(0.0f, -1.0f) };
                    case 3: // left: bottom -> top
                        return { glm::vec2(-hw, -hh + R + t * (m_height - 2.0f * R)),
                                 glm::vec2(0.0f, 1.0f),
                                 glm::vec2(-1.0f, 0.0f) };
                }
            } else {
                // Arc segment - quarter circle, clockwise
                float theta;
                glm::vec2 center;
                int corner = i / 2;

                switch (corner) {
                    case 0: // top-right: theta PI/2 -> 0
                        center = glm::vec2(hw - R, hh - R);
                        theta = 0.5f * PI * (1.0f - t);
                        break;
                    case 1: // bottom-right: theta 0 -> -PI/2
                        center = glm::vec2(hw - R, -hh + R);
                        theta = -0.5f * PI * t;
                        break;
                    case 2: // bottom-left: theta -PI/2 -> -PI
                        center = glm::vec2(-hw + R, -hh + R);
                        theta = -0.5f * PI * (1.0f + t);
                        break;
                    case 3: // top-left: theta PI -> PI/2
                        center = glm::vec2(-hw + R, hh - R);
                        theta = 0.5f * PI * (2.0f - t);
                        break;
                    default:
                        return { glm::vec2(0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 1.0f) };
                }

                glm::vec2 pos = center + R * glm::vec2(cos(theta), sin(theta));
                // Clockwise tangent
                glm::vec2 tangent = glm::vec2(sin(theta), -cos(theta));
                // Outward normal
                glm::vec2 normal = glm::vec2(cos(theta), sin(theta));

                return { pos, tangent, normal };
            }
        }
        cumulative += m_segLen[i];
    }

    return { glm::vec2(-hw + R, hh), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 1.0f) };
}