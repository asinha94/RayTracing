#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <utility>

constexpr int   HEIGHT = 720;
constexpr int   WIDTH  = 720;
constexpr float ASPECT_RATIO = (float) WIDTH / HEIGHT;

constexpr float FOV = 90.f;
const float FOV_MULTIPLIER = std::tan((FOV * M_PI) / 360.f);

constexpr uint8_t COLOR_MAX = 255;
constexpr uint8_t COLOR_MIN = 0;

struct Color {
    uint8_t r,g,b;

    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0): r{r}, g{g}, b{b} {}

    friend std::ofstream& operator<< (std::ofstream& stream, const Color& c) {
        stream << static_cast<int>(c.r) << " "
               << static_cast<int>(c.g) << " "
               << static_cast<int>(c.b) << " ";
        return stream;
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

    void normalize() {
        T sum = x*x + y*y + z*z;
        T mag = std::sqrt(sum);
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
        float proj_internal = std::sqrtf(radius2 - d2);
        float d = proj - proj_internal; // p0 - origin
        return {true, d};
    }

    Color getColor() const { return color;}

private:
    Vec3f center;
    float radius;
    float radius2;
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
//private:
    Vec3f orig;
    Vec3f dir;
};

void render(std::vector<Color>& v, const std::vector<Circle>& objects)
{
    std::cout << "Tracing rays\n";

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            Ray ray{x, y};

            float distance = INFINITY;
            const Circle * obj = nullptr;

            for (const auto& object : objects) {
                auto result = object.intersect(ray.orig, ray.dir);
                if (result.first && result.second < distance) {
                    obj = &object;
                    distance = result.second;
                }
            }

            v[y*WIDTH + x] = obj != nullptr ? obj->getColor() : ColorWhite;
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
    std::vector<Circle> c{
        Circle{Vec3f{0, 0, -10}, ColorRed, 2.f},
        Circle{Vec3f{-2, 2, -15}, ColorBlue, 5.f},
        Circle{Vec3f{-23, 2, -20}, ColorGreen, 5.f}
    };

    std::vector<Color> v(HEIGHT*WIDTH);
    render(v, c);
    draw(fname, v);

    return 0;

}