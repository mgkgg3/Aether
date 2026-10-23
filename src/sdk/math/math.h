#pragma once
#include <cmath>

namespace math
{
	struct vector2 final
	{
		float x{ 0.f }, y{ 0.f };
	};

	struct vector3 final
	{
		float x{ 0.f }, y{ 0.f }, z{ 0.f };

		vector3() : x(0.f), y(0.f), z(0.f) {}
		vector3(float x, float y, float z) : x(x), y(y), z(z) {}

		float& operator[](int i)       { return (&x)[i]; }
		float  operator[](int i) const { return (&x)[i]; }

		vector3 operator+(const vector3& o) const { return { x+o.x, y+o.y, z+o.z }; }
		vector3 operator-(const vector3& o) const { return { x-o.x, y-o.y, z-o.z }; }
		vector3 operator*(float s)          const { return { x*s,   y*s,   z*s   }; }
		vector3& operator+=(const vector3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
		vector3& operator-=(const vector3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }

		float length() const { return std::sqrt(x*x + y*y + z*z); }

		float distance(const vector3& o) const
		{
			float dx = x-o.x, dy = y-o.y, dz = z-o.z;
			return std::sqrt(dx*dx + dy*dy + dz*dz);
		}

		vector3 normalized() const
		{
			float len = length();
			return (len > 0.0001f) ? vector3{x/len, y/len, z/len} : *this;
		}

		float dot(const vector3& o) const { return x*o.x + y*o.y + z*o.z; }

		vector3 cross(const vector3& o) const
		{
			return { y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x };
		}
	};

	struct vector4 final
	{
		float x{ 0.f }, y{ 0.f }, z{ 0.f }, w{ 0.f };
	};

	struct matrix3 final
	{
		union
		{
			float m[9];
			float r[3][3];
		};

		
		
		
		
		
		
		
		
		
		
		
		
		
		vector3 right()   const { return {  m[0],  m[3],  m[6] }; }
		vector3 up()      const { return {  m[1],  m[4],  m[7] }; }
		vector3 forward() const { return { -m[2], -m[5], -m[8] }; }

		static matrix3 identity()
		{
			matrix3 result{};
			result.m[0] = 1.f; result.m[4] = 1.f; result.m[8] = 1.f;
			return result;
		}
	};

	inline vector3 operator*(const matrix3& mat, const vector3& v)
	{
		return
		{
			mat.m[0]*v.x + mat.m[1]*v.y + mat.m[2]*v.z,
			mat.m[3]*v.x + mat.m[4]*v.y + mat.m[5]*v.z,
			mat.m[6]*v.x + mat.m[7]*v.y + mat.m[8]*v.z
		};
	}

	inline bool finite(const vector3& v)
	{
		return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
	}

	inline bool finite(const vector4& v)
	{
		return std::isfinite(v.x) && std::isfinite(v.y) &&
		       std::isfinite(v.z) && std::isfinite(v.w);
	}

	struct matrix4 final
	{
		float m[4][4];

		vector4 multiply(const vector4& v) const
		{
			return {
				m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
				m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
				m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
				m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
			};
		}
	};
}
