#ifndef PERIMETER_H
#define PERIMETER_H

#include <glm/glm.hpp>

struct PerimeterPoint {
    glm::vec2 position;
    glm::vec2 tangent;
    glm::vec2 normal;
};

class Perimeter {
public:
    Perimeter(float width, float height, float cornerRadius = 40.0f);

    float getTotalLength() const { return m_totalLength; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }

    PerimeterPoint getPointAtProgress(float t) const;
    PerimeterPoint getPointAtDistance(float dist) const;

private:
    float m_width, m_height;
    float m_cornerRadius;
    float m_totalLength;
    float m_segLen[8];
};

#endif