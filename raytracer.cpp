#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <utility>

constexpr int   HEIGHT = 720;
constexpr int   WIDTH  = 1280;
constexpr float ASPECT_RATIO = (float) WIDTH / HEIGHT;

constexpr float FOV = 110.f;
const float FOV_MULTIPLIER = std::tan((FOV * M_PI) / 360.f);

constexpr uint8_t COLOR_MAX = 255;
constexpr uint8_t COLOR_MIN = 0;

#ifndef __GNUC__
    using sqrtf = std::sqrtf;
#endif

struct Color {
    uint8_t r,g,b;

    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0): r{r}, g{g}, b{b} {}

    friend std::ofstream& operator<< (std::ofstream& stream, const Color& c) {
        stream << static_cast<int>(c.r) << " "
               << static_cast<int>(c.g) << " "
               << static_cast<int>(c.b) << " ";
        return stream;
    }

    Color operator*(const float scalar) const {
        Color c{*this};
        c.r *= scalar;
        c.g *= scalar;
        c.b *= scalar;
        return c;
    }

};

const Color ColorRed    {COLOR_MAX, COLOR_MIN, COLOR_MIN};
const Color ColorGreen  {COLOR_MIN, COLOR_MAX, COLOR_MIN};
const Color ColorYellow {COLOR_MAX, COLOR_MAX, COLOR_MIN};
const Color ColorBlue   {COLOR_MIN, COLOR_MIN, COLOR_MAX};
const Color ColorBlack  {COLOR_MIN, COLOR_MIN, COLOR_MIN};
const Color ColorWhite  {COLOR_MAX, COLOR_MAX, COLOR_MAX};

template <typename T>
class Vec3 {
public:
    Vec3(): Vec3(0) {}
    Vec3(T x): x{x}, y{x}, z{x} {}
    Vec3(float x, float y, float z): x{x}, y{y}, z{z} {}

    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3& operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vec3& operator*=(const float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vec3 operator+(const Vec3& other) const {
        Vec3 c{*this};
        c += other;
        return c;
    }

    Vec3 operator-(const Vec3& other) const {
        Vec3 c{*this};
        c -= other;
        return c;
    }

    Vec3 operator*(const float scalar) const {
        Vec3 c{*this};
        c *= scalar;
        return c;
    }

    T dot(const Vec3& other) const {
        T t = 0;
        t += this->x * other.x;
        t += this->y * other.y;
        t += this->z * other.z;
        return t;
    }

    T magnitude() const {
        T sum = x*x + y*y + z*z;
        return sqrtf(sum);
    }

    void normalize() {
        T mag = magnitude();
        x /= mag;
        y /= mag;
        z /= mag;
    }

    T x, y, z;
};

using Vec3f = Vec3<float>;


class Circle
{
public:
    Circle(const Vec3f& c, const Color& color, float radius): center{c}, radius{radius}, radius2{radius*radius}, color{color} {}
    std::pair<bool, float> intersect(const Vec3f& origin, const Vec3f& ray) const {
        const Vec3f dir = center - origin;
        float proj = ray.dot(dir);
        
        // Dot product < 0 means vector is pointing
        // in the other direction
        if (proj < 0) return {false, 0};

        // Pythagoras, distance to closest point to center
        float d2 = dir.dot(dir) - (proj * proj);
        // is closest point outside the sphere?
        if (d2 > radius2) return {false, 0};

        // Find distance from closest point to entry point
        float proj_internal = sqrtf(radius2 - d2);
        float d = proj - proj_internal; // p0 - origin
        return {true, d};
    }

    Vec3f center;
    float radius;
    float radius2;
    Color color;
};

class Plane
{
public:
    Plane(): Plane(0, 0, 0) {}
    Plane (const Vec3f& o, const Vec3f& n, Color c) : origin{o}, normal{n}, color{c} {}

    std::pair<bool, float> intersect(const Vec3f& origin, const Vec3f& ray) const {
        float denom = ray.dot(normal);
        if (denom < 1e-6) return {false, 0};

        Vec3f l = this->origin - origin;
        float t = l.dot(normal) / denom;
        if (t < 0) return {false, 0};

        Vec3f v = ray * t;
        return {true, v.magnitude()};
    }


    Vec3f origin;
    Vec3f normal;
    Color color;
};


class Ray
{
public:
    Ray(int x, int y): orig{0} {

        constexpr float invWidth = 1 / static_cast<float>(WIDTH);
        constexpr float invHeight = 1 / static_cast<float>(HEIGHT);

        dir.x = (2 * ((static_cast<float>(x) + 0.5) * invWidth) - 1) * FOV_MULTIPLIER * ASPECT_RATIO;
        dir.y = (1 - 2 * ((static_cast<float>(y) + 0.5) * invHeight)) * FOV_MULTIPLIER;
        dir.z = -1;
        dir.normalize();
    }

    Ray(const Vec3f& orig, const Vec3f& dir): orig{orig}, dir{dir} {} 
    Vec3f orig;
    Vec3f dir;
};

template <typename T>
void calculate_color(Color base_color) {

}

void render(std::vector<Color>& v, const std::vector<Circle>& objects, const std::vector<Plane>& walls)
{
    std::cout << "Tracing rays\n";

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            Ray ray{x, y};

            float distance = INFINITY;
            const Circle * obj = nullptr;

            // Check for first object that intersects
            for (const auto& object : objects) {
                auto result = object.intersect(ray.orig, ray.dir);
                if (result.first && result.second < distance) {
                    obj = &object;
                    distance = result.second;
                }
            }

            const Plane * wall_ptr = nullptr; 

            for (const auto& wall : walls)
            {
                auto result = wall.intersect(ray.orig, ray.dir);
                if (result.first && result.second < distance) {
                    wall_ptr = &wall;
                    distance = result.second;
                }
            }

            // implicit + origin
            Vec3f p0 = ray.dir * distance;

            Vec3f n;
            Color c;
            if (wall_ptr) {
                n = wall_ptr->normal;
                c = wall_ptr->color;
            } else {
                // Get Normal for dot product
                // vector from point to center so normal points in the opposite direction (so we don't have to negate)
                n = obj->center - p0;
                n.normalize();
                c = obj->color;
            }
                
                float dp = n.dot(ray.dir);
                v[y*WIDTH + x] = c * std::max(dp, 0.f);
            
        
        }
    }
}

void draw(std::string fname, const std::vector<Color>& colors)
{
    // Writeout to file
    std::ofstream myfile;
    myfile.open(fname);
    myfile << "P3\n";
    myfile << WIDTH << " " << HEIGHT << "\n" << (int)COLOR_MAX << "\n";
     for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            const Color& c = colors[i*WIDTH + j];
            myfile << c;
        }
        myfile << "\n";
     }
    myfile.close();
}


int main(int argc, char **argv)
{
    std::string fname = argc > 1 ? std::string{argv[1]} : "output.ppm";
    //Circle light{Vec3f{0, 50, 0}, ColorYellow, 10.f};
    std::vector<Circle> spheres{
        Circle{Vec3f{4, 4, -3}, ColorRed, 2.f},
        Circle{Vec3f{-8, 2, -6}, ColorBlue, 2.f},
        Circle{Vec3f{-5, 2, -20}, ColorGreen, 2.f}
    };

    std::vector<Plane> planes{
        Plane{Vec3f{-32,   0, -32}, Vec3f{-1,  0, 0}, Color{200, 200, 200} },
        Plane{Vec3f{ 32,   0, -32}, Vec3f{ 1,  0, 0}, Color{200, 200, 200} },
        Plane{Vec3f{  0,   0, -32}, Vec3f{ 0,  0,-1}, Color{180, 180, 180} },
        Plane{Vec3f{  0,  32, -32}, Vec3f{ 0,  1, 0}, Color{196, 196, 196} },
        Plane{Vec3f{  0, -32, -32}, Vec3f{ 0, -1, 0}, Color{196, 196, 196} },
    };

    std::vector<Color> v(HEIGHT*WIDTH);
    render(v, spheres, planes);
    draw(fname, v);

    return 0;

}